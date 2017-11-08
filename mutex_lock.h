#ifndef FIFO_QUEUE_H
#include "fifo_queue.h"
#endif

#ifndef MUTEX_LOCK_H
#define MUTEX_LOCK_H
typedef struct Lock {
    FIFOq_p queue;
    int flag;
} Lock_s;

typedef Lock_s * Lock_p;

int acquire_lock(Lock_p lock);
int release_lock(Lock_p lock);
int try_lock(Lock_p lock);
#endif
