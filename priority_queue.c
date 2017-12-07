/*
 *
 * Dakota Crane, Dino Hadzic, Tyler Stinson
 * TCSS 422.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "priority_queue.h"

#define ADDITIONAL_ROOM_FOR_TOSTR 4

/*
 * Creates a priority queue.
 *
 * Return: A new priority queue on success, NULL on failure.
 */
PQ_p pq_create() {
    int i, failed = -1;
    PQ_p new_pq = malloc(sizeof(PQ_s));

    if (new_pq != NULL) {
        for (i = 0; i < NUM_PRIORITIES; i++) {
            new_pq->queues[i] = q_create();
            if (new_pq->queues[i] == NULL) {
                failed = i;
                break;
            }
        }
        /* If failed is non-zero, we need to free up everything else. */
        for (i = 0; i <= failed; i++) {
            q_destroy(new_pq->queues[i]);
        }
        /* Simlarly, if it is true, we need to free the priority queue. */
        if (failed != -1) {
            free(new_pq);
            new_pq = NULL;
        }
    }

    return new_pq;
}

/*
 * Destroys the provided priority queue, freeing all contents.
 *
 * Arguments: PQ: The Priority Queue to destroy.
 */
void pq_destroy(PQ_p PQ) {
    int i;

    /* Free all our inner FIFO queues. */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        q_destroy(PQ->queues[i]);
    }

    free(PQ);
}

/*
 * Enqueues a PCB to the provided priority queue, into the correct priority bin.
 *
 * Arguments: PQ: The Priority Queue to enqueue to.
 *            pcb: the PCB to enqueue.
 */
void pq_enqueue(PQ_p PQ, PCB_p pcb) {
    q_enqueue(PQ->queues[pcb->priority], pcb);
}

/*
 * Dequeues a PCB from the provided priority queue.
 *
 * Arguments: PQ: The Priority Queue to dequeue from.
 * Return: The highest priority proccess in the queue, NULL if none exists.
 */
PCB_p pq_dequeue(PQ_p PQ) {
    int i;
    PCB_p ret_pcb = NULL;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!q_is_empty(PQ->queues[i])) {
            ret_pcb = q_dequeue(PQ->queues[i]);
            break;
        }
    }
    return ret_pcb;
}

/*
 * Peeks at the front of the priority queue.
 *
 * Arguments: PQ: the queue to peek.
 * Return: A pointer to the PCB at the front of the queue.
 */
PCB_p pq_peek(PQ_p PQ) {
    int i;
    PCB_p ret_pcb = NULL;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!q_is_empty(PQ->queues[i])) {
            ret_pcb = q_peek(PQ->queues[i]);
            break;
        }
    }
    return ret_pcb;
}

/*
 * Checks if the provided priority queue is empty.
 *
 * Arguments: PQ: The Priority Queue to test.
 * Return: 1 if the queue is empty, 0 otherwise.
 */
char pq_is_empty(PQ_p PQ) {
    int i;
    char ret_val = 1;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        /* If a single queue isn't empty, the priority queue isn't empty. */
        if (q_is_empty(PQ->queues[i]) == 0) {
            ret_val = 0;
            break;
        }
    }

    return ret_val;
}

/*
 * Calculates the number of PCBs in the priority queue.
 *
 * Arguments: PQ: the queue to size.
 * Return: The number of PCBs in the priority queue.
 */
unsigned int pq_size(PQ_p PQ) {
    int i;
    int size = 0;

    for (i = 0; i < NUM_PRIORITIES; i++) {
        size += PQ->queues[i]->size;
    }

    return size;
}

/*
 * Creates a string representation of the provided priority queue, and returns it.
 *
 * Arguments: PQ: the Priority Queue to create a string representation of.
 * Return: A string representation of the provided Priority Queue, or NULL on failure.
 */
char * pq_to_string(PQ_p PQ) {
    unsigned int buff_len = 1000;
    unsigned int cpos = 0;
    unsigned int q_str_len = 0;
    char * ret_str = malloc(sizeof(char) * buff_len);
    char * str_resize = NULL;
    char * q_str = NULL;
    int i;

    if (ret_str != NULL) {
        for (i = 0; i < NUM_PRIORITIES; i++) {
            q_str = q_to_string(PQ->queues[i], 0);
            if (q_str != NULL) {
                q_str_len = strlen(q_str);
                str_resize = resize_block_if_needed(ret_str, cpos + q_str_len
                                        + ADDITIONAL_ROOM_FOR_TOSTR, &buff_len);
                if (str_resize != NULL) {
                    ret_str = str_resize;
                    cpos += sprintf(ret_str + cpos, "Q%d:%s\n", i, q_str);
                }
                free(q_str);
            }
        }
    }

    return ret_str;
}
