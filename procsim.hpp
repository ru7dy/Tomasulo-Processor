#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <cstdio>
#include <vector>

#define DEFAULT_K0 1
#define DEFAULT_K1 2
#define DEFAULT_K2 3
#define DEFAULT_R 8
#define DEFAULT_F 4

typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
    int src_tag[2];
    int dest_tag;
    bool src_ready[2];
    bool fired;
    bool dispatched;
    bool _null;
    int fu_wait;
    int fet_cyc;
    int disp_cyc;
    int sched_cyc;
    int exec_cyc;
    int stateUp_cyc;
    bool completed;
    bool fu_busy;
    bool CDB_busy;
    _proc_inst_t(){
        for(int i=0;i<2;i++){
            src_ready[i]=false;
        }
        dispatched = false;
        completed = false;
        fired = false;
        fu_busy = false;
        CDB_busy = false;
        src_tag[0] = -1;
        src_tag[1] = -1;
        fet_cyc = disp_cyc = sched_cyc = exec_cyc = stateUp_cyc = -1;
        fu_wait = 0;
    }
    // You may introduce other fields as needed
    
} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    float avg_disp_size;
    unsigned long max_disp_size;
    unsigned long retired_instruction;
    unsigned long cycle_count;
} proc_stats_t;

class Schedual_Q
{
public:
    int FU;
    int Dest_reg;
    int Dest_tag;
    int Src_1_reg;
    int Src_1_tag;
    bool Src_1_ready;
    int Src_2_reg;
    int Src_2_tag;
    bool Src_2_ready;
    bool fired;
    bool completed;
    Schedual_Q(){
        Dest_tag = -1;
        Src_1_tag = -1;
        Src_2_tag = -1;
        Src_1_ready = false;
        Src_2_ready = false;
        fired = false;
        completed = false;
    }
    
};


class Function_k
{
public:
    bool busy;
    int tag;
    int dest_reg;
    Function_k(){
        busy = false;
    }
};

class Function_Unit
{
public:
    Function_k* fu_line[20];
    Function_Unit(){
        for(int i=0; i>20; i++)
            fu_line[i] = new Function_k();
    }
    
};
class Result_Bus
{
public:
    bool busy;
    int tag;
    int reg;
    Result_Bus(){
        busy = false;
        tag = -1;
    }
};

class Register_File
{
public:
    bool ready;
    int tag;
    Register_File(){
        ready = true;
        //ready = false;
        tag = -1;
    }
};
bool read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

#endif /* PROCSIM_HPP */
