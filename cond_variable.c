#include "priority_queue.h"
#include "cond_variable.h"



int cond_variable_signal(c_Variable_p var, PCB_p running_process, Lock_p prod_cons_lock, PQ_p ready_queue) {
    
    
    PCB_p waiting = q_dequeue(var->queue);
    if (waiting != NULL) {
        waiting->state = STATE_READY;
        int c = lock(prod_cons_lock, running_process);
        if (c == 0) {
            pq_enqueue(ready_queue, waiting);
        } else {
            lock_trap();
        }
    }
    
}


int cond_variable_wait(Lock_p lock, c_Variable_p var, PCB_p running_process) {
    release_lock(lock);
    q_enqueue(var->queue, running_process); 
}

c_Variable_p cond_variable_constructor() {
    c_Variable_p c_var = malloc(sizeof(cond_variable_s));
    c_var->queue = q_create();
    return c_var;
}
