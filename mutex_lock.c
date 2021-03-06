#include <stdlib.h>
#include "fifo_queue.h"
#include "mutex_lock.h"
#include "pcb.h"
#include <stdio.h>


//// Dakota Crane, Dino Hadzic, Tyler Stinson


Lock_p lock_constructor() {
    Lock_p lock = malloc(sizeof(Lock_s));
    lock->current_proc = NULL;
    lock->waiting_procs = q_create();
    return lock;
}

// this will work as utility to maintain a mapping between a process and its associated lock!
proc_to_lock_map_p proc_map_constructor(Lock_p lock_1, Lock_p lock_2, PCB_p proc) {
    proc_to_lock_map_p proc_lock = malloc(sizeof(proc_to_lock_map_s));
    proc_lock->lock_1 = lock_1;
    proc_lock->lock_2 = lock_2;
    proc_lock->proc = proc;
    return proc_lock;
}

proc_map_list_p proc_map_list_constructor() {
    proc_map_list_p proc_list = malloc(sizeof(proc_map_list_s));
    proc_list->head = NULL;
    proc_list->tail = NULL;
    return proc_list;
}

void proc_map_list_add(proc_map_list_p proc_map, proc_to_lock_map_p map_lock) {
    proc_node_p new_node = malloc(sizeof(proc_node_s));
    new_node->map = map_lock;
    new_node->next = NULL;
    if (proc_map->head == NULL) {
	proc_map->head = new_node;
	proc_map->tail = new_node;
    } else {
	proc_map->tail->next = new_node;
	proc_map->tail = new_node;
    }
}

proc_to_lock_map_p search_list_for_pcb(proc_map_list_p list, PCB_p proc) {
    proc_node_p node = list->head;
    while (node != NULL) {
	if (node->map->proc->pid == proc->pid) {
	    return node->map;
	}
	node = node->next;
    }
    return NULL; // this shouldn't really happen
}

void proc_map_list_destructor(proc_map_list_p proc_map) {
    proc_node_p curr = proc_map->head;
    while (curr != NULL) {
	proc_node_p next = curr->next;
	proc_node_p next_2 = curr->next->next;
	lock_destructor(curr->map->lock_1);
	lock_destructor(curr->map->lock_2);
	free(curr->map);
	free(next->map);
	free(curr);
	free(next);

	curr = next_2;
    }
    free(proc_map);
}

void proc_to_lock_map_destructor(proc_to_lock_map_p proc_map) {
    free(proc_map);
}

int lock(Lock_p lock, PCB_p proc) {
    if (lock->current_proc == NULL) {
	lock->current_proc = proc;
	return 0;
    } else {
	q_enqueue(lock->waiting_procs, proc);
	return 1;
    }
}

int release_lock(Lock_p lock) {
    lock->current_proc = NULL;
    return 0;
}

int try_lock(Lock_p lock, PCB_p proc) {
    if (lock->current_proc == NULL) {
	lock->current_proc = proc;
	return 0;
    } else {
	return 1;
    }
}

void lock_destructor(Lock_p lock) {
    q_destroy(lock->waiting_procs);
    free(lock);
}
