/*
 * Project 1
 *
 * Authors: Keegan Wantz, Carter Odem
 * TCSS 422.
 */

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include "pcb.h"

/* A node used in a fifo queue to store data, and the next node. */
typedef struct node {
    struct node * next;
    PCB_p         pcb;
} Node_s;

typedef Node_s * Node_p;

/* A fifo queue, which stores size and pointers to the first and last nodes. */
typedef struct fifo_queue {
    Node_p first_node;
    Node_p last_node;

    unsigned int size;
} FIFOq_s;

typedef FIFOq_s * FIFOq_p;

/*
 * Create a new FIFO Queue.
 *
 * Return: a pointer to a new FIFO queue, NULL if unsuccessful.
 */
FIFOq_p q_create();

/*
 * Destroy a FIFO queue and all of its internal nodes.
 *
 * Arguments: FIFOq: the queue to destroy.
 * This will also free all PCBs, to prevent any leaks. Do not use on a non empty queue if processing is still going to occur on a pcb.
 */
void q_destroy(/* in-out */ FIFOq_p FIFOq);

/*
 * Checks if a queue is empty.
 *
 * Arguments: FIFOq: the queue to test.
 * Return: 1 if empty, 0 otherwise.
 */
char q_is_empty(/* in */ FIFOq_p FIFOq);

/*
 * Attempts to enqueue the provided pcb.
 *
 * Arguments: FIFOq: the queue to enqueue to.
 *            pcb: the PCB to enqueue.
 * Return: 1 if successful, 0 if unsuccessful.
 */
int q_enqueue(/* in */ FIFOq_p FIFOq, /* in */ PCB_p pcb);

/*
 * Dequeues and returns a PCB from the queue, unless the queue is empty in which case null is returned.
 *
 * Arguments: FIFOq: the queue to dequeue from.
 * Return: NULL if empty, the PCB at the front of the queue otherwise.
 */
PCB_p q_dequeue(/* in-out */ FIFOq_p FIFOq);

/*
 * Peeks at the front of the FIFO queue.
 *
 * Arguments: FIFOq: the queue to peek.
 * Return: A pointer to the PCB at the front of the queue.
 */
PCB_p q_peek(/* in */ FIFOq_p FIFOq);

/*
 * Creates and returns an output string representation of the FIFO queue.
 *
 * Arguments: FIFOq: The queue to perform this operation on
 *            display_back: 1 to display the final PCB, 0 otherwise.
 * Return: a string of the contents of this FIFO queue. User is responsible for
 * freeing consumed memory.
 */
char * q_to_string(/* in */ FIFOq_p FIFOq, /* in */ char display_back);

/*
 * Helper function that resizes a malloced block of memory if the requested
 *   ending position would exceed the block's capacity.
 * I want to pull this into another header/source file, as it can be very useful elsewhere,
 *   but only supposed to submit the 6.
 * Very useful for a variety of things, primarily used in string methods at the moment.
 *
 * Arguments: in_ptr: the pointer to resize.
 *            end_pos: the final desired available position in the resized pointer.
 *            capacity: a pointer to the current capacity, will be changed if resized.
 * Return: NULL if failure, the new (possibly same) pointer otherwise.
 */
void * resize_block_if_needed(/* in */ void * in_ptr, /* in */ unsigned int end_pos, /* in-out */ unsigned int * capacity);


#endif
