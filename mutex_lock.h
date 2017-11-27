#ifndef MUTEX_LOCK_H
#define MUTEX_LOCK_H
#include "fifo_queue.h"
typedef struct Lock {
    PCB_p current_proc;
    FIFOq_p waiting_procs;
} Lock_s;

typedef Lock_s * Lock_p;




typedef struct proc_to_lock_map {
    Lock_p lock_1;
    Lock_p lock_2;
    PCB_p proc;
} proc_to_lock_map_s;

typedef proc_to_lock_map_s * proc_to_lock_map_p;

typedef struct proc_node {
    proc_to_lock_map_p map;
    struct proc_node_s * next;
} proc_node_s;

typedef proc_node_s * proc_node_p;

typedef struct proc_map_list {
    proc_node_s * head;
    proc_node_s * tail;
} proc_map_list_s;

typedef proc_map_list_s * proc_map_list_p;


proc_map_list_p proc_map_list_constructor();
void proc_map_list_add(proc_map_list_p proc_map, proc_to_lock_map_p);
void proc_map_list_destructor(proc_map_list_p proc_map);
void proc_map_destructor(proc_map_list_p proc_map);
Lock_p lock_constructor();
proc_to_lock_map_p proc_map_constructor(Lock_p lock_1, Lock_p lock_2, PCB_p proc);
int lock(Lock_p lock, PCB_p proc);
int release_lock(Lock_p lock);
int try_lock(Lock_p lock, PCB_p proc);
void lock_destructor(Lock_p lock);
proc_to_lock_map_p search_list_for_pcb(proc_map_list_p list, PCB_p proc);
#endif
