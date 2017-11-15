/*
 * TCSS 422 Scheduler Simulation
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pcb.h"
#include "fifo_queue.h"
#include "priority_queue.h"
#include "mutex_lock.h"
#include "cond_variable.h"

/* The number of proccesses (minus one) to generate on initialization. */
#define NUM_PROCESSES 40
#define TEST_ITERATIONS 10000
#define PRIORITY_ZERO_TIME 5 /* Itty bitty quantum sizes for testing. */
#define PER_PRIORITY_TIME_INCREASE 8
#define S_MULTIPLE 8
#define MIN_NUM_BEFORE_TERM 1
#define RANDOM_NUM_BEFORE_TERM 30
#define MIN_MAX_PC_VAL 20 /* Faster than a quantum! */
#define MAX_PC_MODULO 400 /* But can be much longer than a quantum! */
#define NUM_IO_DEVICES 2
#define IO_DELAY_BASE 10
#define IO_DELAY_MOD 100
#define TIMER_SLEEP 1000000


/* Mutexes */
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;

int program_executing;

pthread_t timer_thread;

/* enum for various process states. */
enum interrupt_type {
    INT_NEW,
    INT_TIME,
    INT_IO,
    INT_TERMINATE,


    INTERRUPT_COUNT
};

/* FUNCTIONS */

/* Handles the main execution loop */
int cpu();

/********************
 * INTERRUPTS
 ****************/

/* Interrupt that happens every quantum. */
void pseudo_time_interrupt();
/* Interrupt that happens for IO. */
void io_interrupt(unsigned int io_device);

/***************
 * Interrupt controllers: Basically, returns 1/0 if the interrupt happens.
 **************/

/* Timer "thread" that checks if execution count == quantum size */
void *timer();
/* IO "thread" that checks if the IO timer has hit 0. */
int io_check(unsigned int io_device);

/*****************
 * TRAPS
 ***************/

/* Trap for termination. */
void trap_terminate();
/* Trap for IO */
void trap_io(unsigned int io_device);
/* Tests if the running process should call an IO trap. */
int test_io_trap();

/******************
 * PROCESS HANDLING
 *****************/
/* Schedules items based on the given interrupt type. */
void scheduler(enum interrupt_type type);
/* Dispatches a new process to run. */
void dispatcher();
/* Resets priorities of all processes to 0. */
void handle_priority_reset();


/******************
 * UTILITY
 ***************/

 /* Allocates all system resoucres. */
void initialize_system();
/* Builds a table of minimum cpu */
void build_quantum_times();
/* Generates NUM_PROCESSES PCBs. */
void generate_pcbs();
/* Makes a single PCB. */
PCB_p make_pcb();
/* Tests a process for privileged status in the current simulation. */
int is_privileged(PCB_p pcb);
/* Prints the current state of the queues. */
void print_queue_state();
/* Prints the privileged processes. */
void print_privileged_processes();
/* Print things when an event happens. */
void print_on_event();
/* Deallocates all system resoucres. */
void deallocate_system();


/* GLOBALS */

/* A queue of new processes. */
FIFOq_p new_queue;
/* A queue of processes that are ready to run. */
PQ_p ready_queue;
/* All the processes that are zombies. */
FIFOq_p zombie_queue;
/* Array of IO device queues. */
FIFOq_p io_queues[NUM_IO_DEVICES];
/* Array of IO timers. */
unsigned int io_queue_timers[NUM_IO_DEVICES];

/* The currently running process. */
PCB_p running_process;
/* An array of the amount of cycles alloted to each priority. */
unsigned int quantum_times[NUM_PRIORITIES];
/* The number of cycles since we last reset the prioties. */
unsigned int cpu_cycles_since_reset;
/* The number of cycles that we reset the priority at. */
unsigned int S;
/* Downcounter for the timer device */
unsigned int timer_downcounter;
/* The number of instructions since the cpu started. */
unsigned int current_iteration;
/* CPU PC. */
unsigned int cpu_pc;
/* PC at the top of the system stack. */
unsigned int sys_stack;

/* Main loop. */
int main(void) {
    program_executing = 1;
    
    initialize_system();

    while (program_executing) { // TODO: add trylock for io interrupt lock
        if (pthread_mutex_trylock(&timer_lock) == 0) {
            program_executing = cpu();
            current_iteration++;
            if (current_iteration > TEST_ITERATIONS)
                program_executing = 0;
            pthread_mutex_unlock(&timer_lock);
        }
    }
    pthread_join(timer_thread, NULL);
    deallocate_system();
    return program_executing;
}

/*
 * A single quantum of time.
 *
 * Generates PCBs, 'runs' the program, then interrupts via the timer interrupt.
 */
int cpu() {
    int i;
    /* Flag for if an interrupt fires. */
    int is_interrupt = 0;
    /* Count of CPU instructions since last call to S. */
    cpu_cycles_since_reset++;

    /* Increase the cpu_pc variable to simulate execution. */
    if (running_process != NULL) {
        /* Increase PC: */
        /* We increase these by one; the proc's PC will reflect the one-increment-per-instruction
         * when the proc's PC is updated during context switching */

        cpu_pc += 1;
        if (cpu_pc > running_process->max_pc) {
            cpu_pc = 0;
            running_process->term_count++;
        }
    }

    

    /*
     * IO INTERRUPT: Check for IO completion interrupt.
     * Does not reset is_interrupt, because io int will not change
     * the running process, just bump the IO's queue to the running queue.
     */
    for (i = 0; i < NUM_IO_DEVICES; i++) {
        if (io_check(i)) {
            printf("EVENT: IO Interrupt - ");
            io_interrupt(i);
            print_on_event();
        }
    }


    /* IO TRAP: Check for IO trap */
    if (running_process != NULL) {
        i = test_io_trap();
        if (i) {
            /* Test IO trap returns 1 larger than the IO device to use. */
            i--;
            printf("EVENT: IO Trap Called for PID %u on IO Device %u\n", running_process->pid, i);
            trap_io(i);
        }
    }

    /* TERMINATE TRAP: If the process has been running too long, zombify. */
    if (running_process != NULL && running_process->terminate != 0 && running_process->term_count >= running_process->terminate) {
        printf("EVENT: Terminate Trap Called for PID %u\n", running_process->pid);
        print_on_event();
        trap_terminate();
    }

    /*
     * Idle process
     */
    if (running_process == NULL) {
        scheduler(INT_NEW);
    }

    return 1;
}

/*
 * Pseudo-Time Interrupt Service Routine.
 * Interrupts the running process, then calls the scheduler.
 */
void pseudo_time_interrupt() {
    /* If the process isn't halted, set it to interrupted. */
    if (running_process != NULL && running_process->state != STATE_HALT) {
        PCB_assign_state(running_process, STATE_INT);
        running_process->context->pc = cpu_pc;
    }

    /* Call scheduler. */
    scheduler(INT_TIME);
}

/*
 * Interrupt that happens for IO.
 */
void io_interrupt(unsigned int io_device) {
    PCB_p done_pcb;
    done_pcb = q_dequeue(io_queues[io_device]);
    if (done_pcb != NULL) {
        /* Increment its PC by 1 to prevent it from going back into IO immediately. */
        done_pcb->context->pc++;
        PCB_assign_state(done_pcb, STATE_READY);
        pq_enqueue(ready_queue, done_pcb);

        printf("PID %u ready\n", done_pcb->pid);
        scheduler(INT_IO);
    }
    
}

/*
 * Determines whether it is time to fire the time interrupt or not.
 */
void *timer() { 
    for (;;){
        struct timespec timersleep;
        timersleep.tv_nsec = 1000000;
        nanosleep(0, &timersleep);
        pthread_mutex_lock(&timer_lock);
        printf("EVENT: Timer Interrupt\n");
        print_on_event();
        pseudo_time_interrupt();
        pthread_mutex_unlock(&timer_lock);
        if (program_executing == 0) break;
    }
}

/* IO "thread" that checks if the IO timer has hit 0. */
int io_check(unsigned int io_device) {
    if (!q_is_empty(io_queues[io_device])) {
        io_queue_timers[io_device]--;
        if (io_queue_timers[io_device] == 0)
            return 1;
    }
    return 0;
}

/*
 * IO Trap
 * Pre: The running_process must not be NULL.
 */
void trap_io(unsigned int io_device) {
    running_process->state = STATE_BLOCKED;
    q_enqueue(io_queues[io_device], running_process);
    io_queue_timers[io_device] = quantum_times[running_process->priority] + IO_DELAY_BASE + rand() % IO_DELAY_MOD;
    running_process = NULL;
    print_on_event();
    scheduler(INT_IO);
}

/*
 * Tests if the running process should call an IO trap.
 * Returns 1 if IO set 1, 2 if IO set 2.
 */
int test_io_trap() {
    int i;
    if (running_process != NULL) {
        for (i = 0; i < NUM_IO_TRAPS; i++) {
            if (running_process->io_1_traps[i] == cpu_pc)
                return 1;
            else if (running_process->io_2_traps[i] == cpu_pc)
                return 2;
        }
    }
    return 0;
}


/*
 * Trap for termination.
 * Pre: The running_process must not be NULL.
 */
void trap_terminate() {
    running_process->state = STATE_TERMINATED;
    running_process->termination_time = time(NULL);
    q_enqueue(zombie_queue, running_process);
    running_process = NULL;
    scheduler(INT_TERMINATE);
}

/*
 * The scheduler.
 */
void scheduler(enum interrupt_type type) {
    /* Both values initialized later for speed purposes. */
    PCB_p new_process;
    PCB_p zombie_cleanup;

    /* If more than S cycles have elapsed, reset all processes to highest priority */
    if (cpu_cycles_since_reset >= S) {
        handle_priority_reset();
        /* Set to 0, because subtraction is slower. */
        cpu_cycles_since_reset = 0;
        /* SIMULATION - Add PCBs when S happens */
        generate_pcbs();
        printf("EVENT: Priorities Reset\n");
        print_on_event();
    }

    /*
     * Handle new processes -> ready queue.
     * Running this before the dispatcher
     * to have a higher chance
     * of having a process ready.
     */
    while (!q_is_empty(new_queue)) {
        new_process = q_dequeue(new_queue);
        if (new_process != NULL) {
            PCB_assign_state(new_process, STATE_READY);
            pq_enqueue(ready_queue, new_process);
        }
    }


    /* Handle interrupts. */
    if (running_process != NULL) {
        /* If timer interrupt */
        if (type == INT_TIME) {
            PCB_assign_state(running_process, STATE_READY);
            PCB_assign_priority(running_process, running_process->priority + 1);
            pq_enqueue(ready_queue, running_process);
            printf("EVENT: PID %u ran out of time - moved to ready queue.\n", running_process->pid);
            running_process = NULL;
        }
    }

    if (running_process == NULL) {
        dispatcher();
    }

    /* Handle clearing the zombie queue. */
    if (zombie_queue->size >= 4) {
        while (!q_is_empty(zombie_queue)) {
            zombie_cleanup = q_dequeue(zombie_queue);
            PCB_destroy(zombie_cleanup);
        }
        printf("EVENT: Zombie queue emptied\n");
        print_on_event();
    }
}

/*
 * Dispatches a new process from the ready queue.
 */
void dispatcher() {
    PCB_p dispatch_process = NULL;
    dispatch_process = pq_dequeue(ready_queue);

    if (dispatch_process != NULL) {
        /* Push the process we want to dispatch onto the stack. */
        sys_stack = dispatch_process->context->pc;
        running_process = dispatch_process;
        PCB_assign_state(running_process, STATE_RUNNING);
        printf("EVENT: Dispatch - PID %u is now running\n", running_process->pid);
        print_on_event();
        /* This is simulating popping the top of the SysStack into the CPU PC. */
        cpu_pc = sys_stack;
        /* Set the timer's downcounter to the quantum size of the newly-running proc */
        //timer_downcounter = quantum_times[running_process->priority];
    }
}

/*
 * Resets priotities of all processes to priority 0, to help prevent starvation.
 */
void handle_priority_reset() {
    PCB_p dequeued_proc;
    int i;

    if (running_process != NULL)
        running_process->priority = 0;

    /* Starts at 1, because all in queue 0 are already priority 0. */
    for (i = 1; i < NUM_PRIORITIES; i++) {
        while (!q_is_empty(ready_queue->queues[i])) {
            dequeued_proc = q_dequeue(ready_queue->queues[i]);
            if (dequeued_proc != NULL) {
                PCB_assign_priority(dequeued_proc, 0);
                pq_enqueue(ready_queue, dequeued_proc);
            }
        }
    }
}


/*
 * Initializes all system variables.
 */
void initialize_system() {
    int i;
    /* Seed the RNG. */
    srand(time(NULL));

    /* Make the queues: */
    ready_queue = pq_create();
    zombie_queue = q_create();
    new_queue = q_create();
    for (i = 0; i < NUM_IO_DEVICES; i++) {
        io_queues[i] = q_create();
        io_queue_timers[i] = 0;
    }

    build_quantum_times();

    running_process = NULL;

    current_iteration = 0;
    cpu_pc = 0;
    sys_stack = 0;
    cpu_cycles_since_reset = 0;

    /* Allocate new PCBs and push to new_procceses */
    generate_pcbs();

    pthread_create(&timer_thread, NULL, timer, NULL); // TODO: move to right place
}

/*
 * Builds a table of cpu cycle times alloted to each quantum.
 */
void build_quantum_times() {
    int i;
    for (i = 0; i < NUM_PRIORITIES; i++) {
        quantum_times[i] = PRIORITY_ZERO_TIME + PER_PRIORITY_TIME_INCREASE * i;
    }

    S = quantum_times[NUM_PRIORITIES/2] * S_MULTIPLE;
}

/*
 * Generates PCBs for populating the new_queue.
 */
void generate_pcbs() {
    int i;
    int num_to_make, lottery;
    PCB_p new_pcb = NULL;

    num_to_make = rand() % NUM_PROCESSES;

    for (i = 0; i < num_to_make; i++) {
        new_pcb = make_pcb();
        /*
         * Randomly decide if one process will be not terminate or not.
         */
        lottery = rand() % 1000;
        if (lottery <= 5) {
            new_pcb->terminate = 0;
        }

        if (new_pcb != NULL) {
            q_enqueue(new_queue, new_pcb);
        }
    }
}

/*
 * Makes a new PCB and returns it.
 */
PCB_p make_pcb() {
    int i;
    PCB_p my_pcb = PCB_create();

    if (my_pcb != NULL) {
        PCB_assign_PID(my_pcb);
        PCB_assign_priority(my_pcb, 0);
        /* Assigns parent for testing purposes. */
        PCB_assign_parent(my_pcb, my_pcb->pid-1);

        time_t current_time = time(NULL);
        my_pcb->creation_time = current_time;
        /* Set the max_pc.. */
        my_pcb->max_pc = MIN_MAX_PC_VAL + rand() % MAX_PC_MODULO;
        /* Start the PC at some value < max_pc for testing. */
        my_pcb->context->pc = rand() % my_pcb->max_pc;
        /*
         * Set the number of runs before termination to a random number between
         * MIN_NUM_BEFORE_TERM and MIN_NUM_BEFORE_TERM + RANDOM_NUM_BEFORE_TERM + 1
         */
        my_pcb->terminate = MIN_NUM_BEFORE_TERM + rand() % RANDOM_NUM_BEFORE_TERM;

        for (i = 0; i < NUM_IO_TRAPS; i++) {
            my_pcb->io_1_traps[i] = rand() % (my_pcb->max_pc/NUM_IO_TRAPS);
            my_pcb->io_2_traps[i] = rand() % (my_pcb->max_pc/NUM_IO_TRAPS);
            /* If we're past the first io trap, add the previous value to this one. */
            if (i > 0) {
                my_pcb->io_1_traps[i] = my_pcb->io_1_traps[i] + my_pcb->io_1_traps[i-1];
                my_pcb->io_2_traps[i] = my_pcb->io_2_traps[i] + my_pcb->io_2_traps[i-1];
            }
        }
    }
    return my_pcb;
}

/*
 * Prints the current state of the queues:
 */
void print_queue_state() {
    unsigned int i;
    for (i = 0; i < NUM_PRIORITIES; i++) {
        printf("Q%u: %u\t- Quantum Size: %u\n", i, ready_queue->queues[i]->size, quantum_times[i]);
    }

    for (i = 0; i < NUM_IO_DEVICES; i++) {
        if (!q_is_empty(io_queues[i])) {
            printf("IO Device %u queue contains %u PCBs.\n", i, io_queues[i]->size); 
            printf("Head of IO device %u queue: PID%u \n", i, q_peek(io_queues[i])->pid);
        }
    }
}

/*
 * Prints everything needed on an event.
 */
void print_on_event() {
    if (running_process != NULL) {
        printf("Running: PID %u, PRIORITY %u, PC %u\n", running_process->pid, running_process->priority, running_process->context->pc);
    }
    printf("Current Iteration: %u\n", current_iteration);
    print_queue_state();

    printf("\n");
}

/*
 * Cleans up all system variables.
 */
void deallocate_system() {
    int i;

    /* Cleanup: */
    pq_destroy(ready_queue);
    q_destroy(zombie_queue);
    q_destroy(new_queue);

    for (i = 0; i < NUM_IO_DEVICES; i++) {
        q_destroy(io_queues[i]);
        io_queue_timers[i] = 0;
    }

    if (running_process != NULL)
        PCB_destroy(running_process);
}
