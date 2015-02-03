#include "procsim.hpp"
#include <vector>
#include <cstdio>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

unsigned long disp_q_size = 0;
unsigned long max_disp_q_size = 0;
unsigned long sum_disp = 0;
int result[100000][6];
int seq = 0;
int CDB_Size;
int Fu_Size[3];
int FU_0_Size;
int FU_1_Size;
int FU_2_Size;
int Fetch_rate;
int Schedule_Q_Size;
int Num_RS; // number of Reservation Slots
int cycle = 1;
int line = 0;
int disp_start = 0;
int FU_aval[3];
int CDB_aval;
int CDB_in_vector[12];
Function_Unit FU[3];
Register_File RF[129]; // RF[128] for -1
std::vector<_proc_inst_t*> input_q;
std::vector<_proc_inst_t*> output_q;
std::vector<_proc_inst_t*> inst_q;
int fu_line_num;
typedef std::vector<proc_inst_t*>::iterator inst_q_iterator;
void Fetch();
void RF_update();
void Execute();
void Schedule_fire();
void Dispatch();
void Schedule_update();
void Delete_inst_from_sched_q();
void count_disp_q_size();

bool in_disp_q(_proc_inst_t *inst);
bool FU_busy(_proc_inst_t *inst);
bool in_sched_q_before_fire(_proc_inst_t *inst);




/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @r ROB size
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 */

_proc_inst_t* read_instruction()
{
    _proc_inst_t *p_inst = new _proc_inst_t();
    int         ret = fscanf(stdin, "%x %d %d %d %d", &p_inst->instruction_address,
                             &p_inst->op_code, &p_inst->dest_reg, &p_inst->src_reg[0], &p_inst->src_reg[1]);
    if (ret != 5){
        p_inst->_null = true;
        return p_inst;
    }else{
        p_inst->_null = false;
    }
    
/* This is where I deleted my read instruct function, so the submitted file will do run on register -1 */
    for(int i=0; i<2; i++){
        if(p_inst->src_reg[i] == -1)
            p_inst->src_reg[i] = 128;
    }
/* end */
    
    if(p_inst->op_code == -1)
        p_inst->op_code = 1;
    p_inst->dest_tag = line;
    p_inst->fet_cyc = 1 + line/Fetch_rate;
    p_inst->disp_cyc = p_inst->fet_cyc + 1;
    line++;
    return p_inst;
}

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f) 
{
    CDB_Size = r;
    CDB_aval = r;
    Fu_Size[0] = k0;
    Fu_Size[1]= k1;
    Fu_Size[2] = k2;
    Schedule_Q_Size = 2 * (k0 + k1 + k2);
    Fetch_rate = f;
    Num_RS = Schedule_Q_Size;
    FU_aval[0] = k0;
    FU_aval[1] = k1;
    FU_aval[2] = k2;


}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats)
{
    do{
        Execute();
        RF_update();
        Schedule_fire();
        Dispatch();
        Fetch();
        count_disp_q_size();
        Schedule_update();
        Delete_inst_from_sched_q();
        cycle++;
    }while(!inst_q.empty());
    
}

void count_disp_q_size(){
    if(Fetch_rate * cycle <= 100000){
        disp_q_size += Fetch_rate;
        if(disp_q_size > max_disp_q_size)
            max_disp_q_size = disp_q_size;
    }
    sum_disp += disp_q_size;
}

void Fetch(){
    for(int i = 0; i < (Fetch_rate < Num_RS ? Fetch_rate : Num_RS ); i++){
        _proc_inst_t *inst = read_instruction();
        if(inst->_null){
            return;
        }
        else{
            inst_q.push_back(inst);
        }
    }
}

void Dispatch(){
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){ // for every inst. in disp_q
        if(in_disp_q((*i))){
            if(Num_RS){
                
                disp_q_size -= 1;
                Num_RS--;
                for(int j = 0; j < 2; j++){
                    if(RF[(*i)->src_reg[j]].ready){
                        (*i)->src_ready[j] = true;
                    }
                    else{
                        (*i)->src_tag[j] = RF[(*i)->src_reg[j]].tag;
                        (*i)->src_ready[j] = false;
                    }
                }
                if((*i)->dest_reg != -1){
                    RF[(*i)->dest_reg].tag = (*i)->dest_tag;
                    RF[(*i)->dest_reg].ready = false;
                }
                else{
                    RF[128].tag = (*i)->dest_tag;
                    
                    /* here is supposed to be commented! */
                    //RF[128].ready = false;
                    /* end */
                    
                }
                (*i)->dispatched = true;
                (*i)->sched_cyc = cycle + 1;
            }
        }
    }
}

bool in_disp_q(_proc_inst_t *inst){
    return (inst->disp_cyc <= cycle && (inst->disp_cyc != -1)
            && inst->sched_cyc == -1 && inst->exec_cyc == -1 && inst->stateUp_cyc == -1 && !inst->dispatched) ;
}

void Schedule_fire(){
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){ // for every inst. in sched_q bofore fire
        if(in_sched_q_before_fire((*i))){
            if((*i)->src_ready[0] == true
               && (*i)->src_ready[1] == true
               && FU_aval[(*i)->op_code]){
                (*i)->fired = true;
                (*i)->fu_busy = true;
                FU_aval[(*i)->op_code]--;
                (*i)->exec_cyc = cycle + 1;
            }
        }
    }
}

bool in_sched_q_before_fire(_proc_inst_t *inst){
    return ((inst->sched_cyc <= cycle) && (inst->sched_cyc != -1) && (inst->fired == false));
}

void Schedule_update(){
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
        if((*i)->CDB_busy){
                for(inst_q_iterator j = inst_q.begin(); j != inst_q.end(); j++){
                    if((*j)->src_tag[0] == (*i)->dest_tag){
                        (*j)->src_ready[0] = true;
                    }
                    if((*j)->src_tag[1] == (*i)->dest_tag){
                        (*j)->src_ready[1] = true;
                    }
                }
            (*i)->CDB_busy = false;
            CDB_aval++;
            }
    
      }
  
}



void Execute(){
    int max_fu_wait = 0;
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
        if((*i)->fu_wait > max_fu_wait){
            max_fu_wait = (*i)->fu_wait;
        }
    }
    while(max_fu_wait){
        for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
            if(CDB_aval && (*i)->fu_wait == max_fu_wait){
                CDB_aval--;
                FU_aval[(*i)->op_code]++;
                (*i)->fu_busy = false;
                (*i)->fu_wait = 0;
                (*i)->CDB_busy = true;
                (*i)->completed = true;
                (*i)->stateUp_cyc = cycle + 1;
            }
        }
        max_fu_wait--;
    }
    if(CDB_aval){
        for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
            if(CDB_aval && (*i)->exec_cyc != -1 && ((*i)->exec_cyc) <= cycle && !(*i)->completed){
                CDB_aval--;
                FU_aval[(*i)->op_code]++;
                (*i)->fu_busy = false;
                (*i)->fu_wait = 0;
                (*i)->CDB_busy = true;
                (*i)->completed = true;
                (*i)->stateUp_cyc = cycle + 1;
            }
        }
    }

    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
        if((*i)->fired && !(*i)->completed){
            (*i)->fu_wait++;
            
        }
    }
}

void RF_update(){
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); i++){
        if((*i)->CDB_busy && (*i)->dest_tag == RF[(*i)->dest_reg].tag){
            if((*i)->dest_reg != -1){
                RF[(*i)->dest_reg].ready = true;
            }
            else{
                RF[128].ready = true;
            }
            
        }
    }
}

void Delete_inst_from_sched_q(){
    for(inst_q_iterator i = inst_q.begin(); i != inst_q.end(); ){
            if((*i)->completed){
                Num_RS++;
                result[(*i)->dest_tag][0] = (*i)->dest_tag + 1;
                result[(*i)->dest_tag][1] = (*i)->fet_cyc;
                result[(*i)->dest_tag][2] = (*i)->disp_cyc;
                result[(*i)->dest_tag][3] = (*i)->sched_cyc;
                result[(*i)->dest_tag][4] = (*i)->exec_cyc;
                result[(*i)->dest_tag][5] = (*i)->stateUp_cyc;
                i = inst_q.erase(i);
            }
        else{
            ++i;
        }
    }
}
/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats) 
{
    printf("%s\t%s\t%s\t%s\t%s\t%s\n", "INST", "FETCH", "DISP", "SCHED", "EXEC", "STATE");
    for(int i = 0; i < 100000; i++){
        for(int j = 0; j < 6; j++){
            printf("%i\t", result[i][j]);
        }
        printf("\n");
    }
    
    p_stats->retired_instruction = line ;
    p_stats->cycle_count = cycle ;
    p_stats->avg_inst_fired = ((double) line) / ((double) cycle);
    p_stats->avg_inst_retired = ((double) line) / ((double) cycle);
    p_stats->max_disp_size = max_disp_q_size;
    p_stats->avg_disp_size = ((double) sum_disp) / ((double) cycle);

}
