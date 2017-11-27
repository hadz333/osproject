/*
 * Project 1
 *
 * Authors: Keegan Wantz, Carter Odem
 * TCSS 422.
 */

#include"pcb.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int global_largest_PID = 0;

/*
 * Helper function to iniialize PCB data.
 */
void initialize_data(/* in-out */ PCB_p pcb) {
  pcb->pid = 0;
  pcb->priority = 0;
  pcb->size = 0;
  pcb->channel_no = 0;
  pcb->state = STATE_NEW;

  pcb->max_pc = 0;
  pcb->creation_time = 0;
  pcb->termination_time = 0;
  pcb->terminate = 0;
  pcb->term_count = 0;

  pcb->mem = NULL;

  pcb->context->pc = 0;
  pcb->context->ir = 0;
  pcb->context->r0 = 0;
  pcb->context->r1 = 0;
  pcb->context->r2 = 0;
  pcb->context->r3 = 0;
  pcb->context->r4 = 0;
  pcb->context->r5 = 0;
  pcb->context->r6 = 0;
  pcb->context->r7 = 0;
}

/*
 * Allocate a PCB and a context for that PCB.
 *
 * Return: NULL if context or PCB allocation failed, the new pointer otherwise.
 */
PCB_p PCB_create() {
    PCB_p new_pcb = malloc(sizeof(PCB_s));
    if (new_pcb != NULL) {
        new_pcb->context = malloc(sizeof(CPU_context_s));

	// is anything beyond the for and necessary here? Wouldn't we run into a stack overflow if this fasiled
        if (new_pcb->context != NULL && new_pcb->io_1_traps != NULL && new_pcb->io_2_traps != NULL) {
          initialize_data(new_pcb);
        } else {
            free(new_pcb);
            new_pcb = NULL;
        }
    }
    return new_pcb;
}

/*
 * Frees a PCB and its context.
 *
 * Arguments: pcb: the pcb to free.
 */
void PCB_destroy(/* in-out */ PCB_p pcb) {
  free(pcb->context);
  free(pcb);
}

/*
 * Assigns intial process ID to the process.
 *
 * Arguments: pcb: the pcb to modify.
 */
void PCB_assign_PID(/* in */ PCB_p the_PCB) {
    the_PCB->pid = global_largest_PID;
    global_largest_PID++;
}

/*
 * Sets the state of the process to the provided state.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new state of the process.
 */
void PCB_assign_state(/* in-out */ PCB_p the_pcb, /* in */ enum state_type the_state) {
    the_pcb->state = the_state;
}

/*
 * Sets the parent of the given pcb to the provided pid.
 *
 * Arguments: pcb: the pcb to modify.
 *            pid: the parent PID for this process.
 */
void PCB_assign_parent(PCB_p the_pcb, int the_pid) {
    the_pcb->parent = the_pid;
}

/*
 * Sets the priority of the PCB to the provided value.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new priority of the process.
 */
void PCB_assign_priority(/* in */ PCB_p the_pcb, /* in */ unsigned int the_priority) {
    the_pcb->priority = the_priority;
    if (the_priority >= NUM_PRIORITIES) {
        the_pcb->priority = NUM_PRIORITIES - 1;
    }
}

/*
 * Create and return a string representation of the provided PCB.
 *
 * Arguments: pcb: the pcb to create a string representation of.
 * Return: a string representation of the provided PCB on success, NULL otherwise.
 */
char * PCB_to_string(/* in */ PCB_p the_pcb) {
    /* Oversized buffer for creating the initial version of the string. */
    char temp_buf[1000];
    unsigned int cpos = 0;

    cpos += sprintf(temp_buf, "contents: PID: 0x%X, Priority: 0x%X, state: %u, "
            "memloc: %p size: %u channel: %X ",
            the_pcb->pid, the_pcb->priority, the_pcb->state,
            the_pcb->mem, the_pcb->size, the_pcb->channel_no);

    /* Append the context: */
    sprintf(temp_buf + cpos, "PC: 0x%04X, IR: %04X, "
            "r0: %04X, r1: %04X, r2: %04X, r3: %04X, r4: %04X, "
            "r5: %04X, r6: %04X, r7: %04X",
            the_pcb->context->pc, the_pcb->context->ir, the_pcb->context->r0,
            the_pcb->context->r1, the_pcb->context->r2, the_pcb->context->r3,
            the_pcb->context->r4, the_pcb->context->r5, the_pcb->context->r6,
            the_pcb->context->r7);

    /* A string that can be returned and -not- go out of scope. */
    char * ret_val = malloc(sizeof(char) * (strlen(temp_buf) + 1));

    /* Make sure ret_val is not null before populating it. */
    if (ret_val != NULL) {
        strcpy(ret_val, temp_buf);
    }

    return ret_val;
}
