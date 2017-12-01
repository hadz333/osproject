#ifndef COND_VARIABLE_H
#define COND_VARIABLE_H
#include "fifo_queue.h"
#include "mutex_lock.h"

typedef struct _Cond_Variable {
    FIFOq_p queue;
     
} cond_variable_s;

typedef cond_variable_s * c_Variable_p;

int cond_variable_signal(c_Variable_p var, PCB_p running_process, Lock_p prod_cons_lock, PQ_p ready_queue);
int cond_variable_wait(Lock_p lock, c_Variable_p var, PCB_p running_process);
c_Variable_p cond_variable_constructor();

#endif