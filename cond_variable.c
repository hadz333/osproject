#include "priority_queue.h"
#include "cond_variable.h"



int cond_variable_signal(c_Variable_p var, PCB_p running_process, Lock_p prod_cons_lock, PQ_p ready_queue) {
    
    
    PCB_p waiting = q_dequeue(var->queue);
    if (waiting != NULL) {
        int c = lock(prod_cons_lock, waiting);
        if (c == 0) {
            waiting->state = STATE_READY;
            pq_enqueue(ready_queue, waiting);
            
        } 
        /*
        else {
            q_enqueue(var->queue, waiting);
                
            lock_trap();
            return; 
        }
        */
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

void c_var_destructor(c_Variable_p var) {
    q_destroy(var->queue);
    free(var);
}
