#ifndef FIFO_QUEUE_H
#include "fifo_queue.h"
#endif

#ifndef MUTEX_LOCK_H
#include "mutex_lock.h"
#endif

#ifndef COND_VARIABLE_H
#define COND_VARIABLE_H

typedef struct _Cond_Variable {
    FIFOq_p queue;
} Cond_Variable_s;

typedef Cond_Variable_s * c_Variable;

int cond_variable_signal(c_Variable var);
int cond_variable_wait(Lock_p lock, c_Variable var);

#endif
