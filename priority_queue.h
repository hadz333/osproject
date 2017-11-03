/*
 * Project 1
 *
 * Authors: Keegan Wantz, Carter Odem
 * TCSS 422.
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "pcb.h"
#include "fifo_queue.h"

typedef struct priority_queue {
    FIFOq_p     queues[NUM_PRIORITIES];
} PQ_s;

typedef struct priority_queue * PQ_p;


/*
 * Creates a priority queue.
 *
 * Return: A new priority queue on success, NULL on failure.
 */
PQ_p pq_create();

/*
 * Destroys the provided priority queue, freeing all contents.
 *
 * Arguments: PQ: The Priority Queue to destroy.
 */
void pq_destroy(PQ_p PQ);

/*
 * Enqueues a PCB to the provided priority queue, into the correct priority bin.
 *
 * Arguments: PQ: The Priority Queue to enqueue to.
 *            pcb: the PCB to enqueue.
 */
void pq_enqueue(PQ_p PQ, PCB_p pcb);
/*
 * Dequeues a PCB from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to dequeue from.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
PCB_p pq_dequeue(PQ_p PQ);

/*
 * Peeks at the front of the priority queue.
 *
 * Arguments: PQ: the queue to peek.
 * Return: A pointer to the PCB at the front of the queue.
 */
PCB_p pq_peek(PQ_p PQ);

/*
 * Calculates the number of PCBs in the priority queue.
 *
 * Arguments: PQ: the queue to size.
 * Return: The number of PCBs in the priority queue.
 */
unsigned int pq_size(PQ_p PQ);

/*
 * Checks if the provided priority queue is empty.
 *
 * Arguments: PQ: The Priority Queue to test.
 * Return: 1 if the queue is empty, 0 otherwise.
 */
char pq_is_empty(PQ_p PQ);

/*
 * Creates a string representation of the provided priority queue, and returns it.
 *
 * Arguments: PQ: the Priority Queue to create a string representation of.
 * Return: A string representation of the provided Priority Queue, or NULL on failure.
 */
char * pq_to_string(PQ_p PQ);

#endif
