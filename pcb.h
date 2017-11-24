/*
 * Project 1
 *
 * Authors: Keegan Wantz, Carter Odem
 * TCSS 422.
 */
#include <time.h>

#ifndef PCB_H  /* Include guard */
#define PCB_H

#define NUM_PRIORITIES 16
#define NUM_IO_TRAPS 4 // number of pc triggers in io trap arrays



/* The CPU state, values named as in the LC-3 processor. */
typedef struct cpu_context {
    unsigned int pc;
    unsigned int ir;
    unsigned int psr;
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
} CPU_context_s; // _s means this is a structure definition

typedef CPU_context_s * CPU_context_p; // _p means that this is a pointer to a structure

enum proc_type {
    IO,
    INTENSIVE,
};
/* enum for various process states. */
enum state_type {
    /* New process. */
    STATE_NEW,
    /* Ready to execute. */
    STATE_READY,
    /* Executing */
    STATE_RUNNING,
    /* Interrupted */
    STATE_INT,
    /* Thread wait? */
    STATE_WAIT,
    /* I/O */
    STATE_BLOCKED,
    /* Halted */
    STATE_HALT,
    /* Terminated */
    STATE_TERMINATED,
};

/* Process Control Block - Contains info required for executing processes. */
typedef struct pcb {
    unsigned int pid; // process identification
    enum state_type state; // process state (running, waiting, etc.)
    unsigned int parent; // parent process pid
    unsigned char priority; // 0 is highest – 15 is lowest.
    unsigned char * mem; // start of process in memory
    unsigned int size; // number of bytes in process
    unsigned char channel_no; // which I/O device or service Q
    // if process is blocked, which queue it is in
    CPU_context_p context; // set of cpu registers
    unsigned int max_pc; // max number of instructions to process before reset
    time_t creation_time; // system time of process creation
    time_t termination_time; // system of of process termination, if relevant
    unsigned int terminate; // control field - how many runs until proc terminates
    unsigned int term_count; // counter - how many times has proc passed max_pc value
    unsigned int io_1_traps[NUM_IO_TRAPS];
    unsigned int io_2_traps[NUM_IO_TRAPS];

    enum proc_type proc_type;

    // other items to be added as needed.

} PCB_s;

typedef PCB_s * PCB_p;

/*
 * Allocate a PCB and a context for that PCB.
 *
 * Return: NULL if context or PCB allocation failed, the new pointer otherwise.
 */
PCB_p PCB_create();

/*
 * Frees a PCB and its context.
 *
 * Arguments: pcb: the pcb to free.
 */
void PCB_destroy(/* in-out */ PCB_p pcb);

/*
 * Assigns intial process ID to the process.
 *
 * Arguments: pcb: the pcb to modify.
 */
void PCB_assign_PID(/* in */ PCB_p pcb);

/*
 * Sets the state of the process to the provided state.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new state of the process.
 */
void PCB_assign_state(/* in-out */ PCB_p pcb, /* in */ enum state_type state);

/*
 * Sets the parent of the given pcb to the provided pid.
 *
 * Arguments: pcb: the pcb to modify.
 *            pid: the parent PID for this process.
 */
void PCB_assign_parent(PCB_p the_pcb, int pid);

/*
 * Sets the priority of the PCB to the provided value.
 *
 * Arguments: pcb: the pcb to modify.
 *            state: the new priority of the process.
 */
void PCB_assign_priority(/* in */ PCB_p pcb, /* in */ unsigned int priority);

/*
 * Create and return a string representation of the provided PCB.
 *
 * Arguments: pcb: the pcb to create a string representation of.
 * Return: a string representation of the provided PCB on success, NULL otherwise.
 */
char * PCB_to_string(/* in */ PCB_p pcb);

#endif
