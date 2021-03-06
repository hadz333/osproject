/*
 * TCSS 422 Scheduler Simulation
 * Dakota Crane, Dino Hadzic, Tyler Stinson
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
#define TEST_ITERATIONS 1000000
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
#define TIMER_SLEEP 10000000


#define NUM_TYPE_PROCS 4
#define MAX_IO_PROCS 50
#define MAX_INTENSIVE_PROCS 25
#define MAX_MUTEX_PROCS 50

#define MAX_PROD_CONS_PROC_PAIRS 10
#define DEADLOCK_CHECK_THRESHOLD 10 //how many context switches before checking for deadlock
#define CREATE_DEADLOCK_TRUE 1 // change to 1 if you want deadlock

int count_io_procs = 0;
int count_comp_procs = 0;
int count_mutex_procs = 0;
int count_terminated = 0;

int count_prod_cons_procs = 0;
int prod_cons_globals[10][2]; // second dimension index 0 is counter incremented, index 1 is flip

c_Variable_p prod_cons_cond_vars[10][2]; // second dimension index 0 is fill, index 1 is empty
Lock_p prod_cons_locks[10];

int curr_prod_cons_id = 0;

int io_total = 0;
int intensive_total = 0;
int mutex_total = 0;

proc_map_list_p list_of_locks;
int deadlock_check_counter = 0;
int deadlock_flag = -1;


proc_map_list_p list_of_locks;

/* Mutexes */
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t io_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cpu_cond = PTHREAD_COND_INITIALIZER;


// used when an io device needs to wait on either another io device or the timer itself
// when a timer or io device finished their routine, this should run
pthread_cond_t io_cond = PTHREAD_COND_INITIALIZER;

pthread_cond_t io_trap_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t io_cond_1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t io_cond_2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t timer_init_lock = PTHREAD_COND_INITIALIZER;
pthread_mutex_t io_init_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int * one = 0;
unsigned int * two = 0;


int program_executing;
int initialized_cond = 0;
int initialized_io = 0;

pthread_t timer_thread;
pthread_t io_thread_1;
pthread_t io_thread_2;

/* enum for various process states. */
enum interrupt_type {
    INT_NEW,
    INT_TIME,
    INT_IO,
    INT_TERMINATE,
    TRAP_IO,
    INTERRUPT_COUNT,
    TRAP_PROD_CONS,
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
void *io_interrupt(unsigned int * io_device);

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


void prod_cons_trap();

/******************
 * PROCESS HANDLING
 *****************/
/* Schedules items based on the given interrupt type. */
void scheduler(enum interrupt_type type);
/* Dispatches a new process to run. */
void dispatcher();
/* Resets priorities of all processes to 0. */
void handle_priority_reset();

void lock_thread_by_priority(enum interrupt_type type);

int exists(unsigned int check, unsigned int arr[], int size, PCB_p proc);


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
/* Monitors for deadlock */
int deadlock_monitor();
/* Prints the current state of the queues. */
void print_queue_state();
/* Prints the privileged processes. */
void print_privileged_processes();
/* Print things when an event happens. */
void print_on_event();
/* Deallocates all system resoucres. */
void deallocate_system();

int exists_in_range(unsigned int base, unsigned int bound, unsigned int check);

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

int contains(unsigned int arr[], unsigned int num, int size);
void unlock_and_release_waiting_procs(Lock_p lock);
void lock_trap(Lock_p lock);

/* Main loop. */
int main(void) {
    list_of_locks = proc_map_list_constructor();

    program_executing = 1;
    
    initialize_system();

    while (program_executing) { 
        if (pthread_mutex_trylock(&timer_lock) == 0) { {
            program_executing = cpu();
            current_iteration++;
            if (current_iteration > TEST_ITERATIONS)
                program_executing = 0;
            pthread_mutex_unlock(&timer_lock);
        }
    }
    pthread_join(timer_thread, NULL);
    pthread_cond_signal(&io_cond_1);
    pthread_cond_signal(&io_cond_2);
    pthread_join(io_thread_1, NULL);
    pthread_join(io_thread_2, NULL);

    deallocate_system();

    proc_map_list_destructor(list_of_locks);
    int k;

    for (k = 0; k < 10; k++) {
	if (prod_cons_locks[k] != NULL) {
	    lock_destructor(prod_cons_locks[k]);
	    c_var_destructor(prod_cons_cond_vars[k][0]);
	    c_var_destructor(prod_cons_cond_vars[k][1]);
	}
    }

    if (deadlock_flag == -1) {
        printf("Run finished. No deadlock occurred during run\n");
    } else {
        printf("Run finished. Deadlock occurred at least once\n");
    }
    printf("Num IO processes: %i\n", io_total);
    printf("Num intensive processes: %i\n", intensive_total);
    printf("Num mutual resource processes: %i\n", mutex_total);
    printf("Num prod/con processes: %i\n", count_prod_cons_procs* 2);

    printf("Total number of processes created: %u\n", io_total + intensive_total + (mutex_total * 2) + (count_prod_cons_procs*2));
    printf("Total number of processes terminated:%u\n", count_terminated);

    return program_executing;
}

/*
 * A single quantum of time.
 *
 * Generates PCBs, 'runs' the program, then interrupts via the timer interrupt.
 */
int cpu() {
    int i;
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

	if (deadlock_check_counter >= DEADLOCK_CHECK_THRESHOLD) {
	    deadlock_monitor();
	    deadlock_check_counter = 0;
	}

    }

    // consider different kinds of procs!

    /* IO TRAP: Check for IO trap */
    if (running_process != NULL) {
	switch (running_process->proc_type) {
	case INTENSIVE: 
	    break;
	case MUTEX:
	    if (contains(running_process->lock_1, cpu_pc, 4) == 1) {
		proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
		PCB_p lockedproc = map->lock_1->current_proc;
		if (lockedproc != NULL) {
		    printf("lock 1 has process before lock attempt = %u, running procces=%u\n", lockedproc->pid, running_process->pid);
		}
		//if (lockedproc != NULL)
		    //printf("lock 1 has process pid=%u, running proc pid=%u\n", lockedproc->pid, running_process->pid);
		int attempt = lock(map->lock_1, running_process);
		if (attempt == 0) {
	    	    //printf("LOCK 1 proc pid: %u - pc: %u \n", running_process->pid, cpu_pc);
	    	    printf("PID %u: requested lock on mutex 1 - succeeded\n", running_process->pid);
		} else {
		    //printf("sleeping lock 1, pid %u\n", running_process->pid);
		    printf("PID %u: requested lock on mutex 1 - blocked by PID %u\n", running_process->pid, lockedproc->pid);
		    
		    lock_trap(map->lock_1);
		}
	    } else if (contains(running_process->lock_2, cpu_pc, 4) == 1) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
		PCB_p lockedproc = map->lock_2->current_proc;
	    	int attempt = lock(map->lock_2, running_process);
	    	if (attempt == 0) {
	    	    printf("PID %u: requested lock on mutex 2 - succeeded\n", running_process->pid);
	    	} else {
		    printf("PID %u: requested lock on mutex 2 - blocked by PID %u\n", running_process->pid, lockedproc->pid);
	    	    lock_trap(map->lock_2);
	    	}
	    } else if (contains(running_process->unlock_1, cpu_pc, 4) == 1) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	if (map->proc != NULL && map->proc == running_process) {
	    	    release_lock(map->lock_1);
	    	    unlock_and_release_waiting_procs(map->lock_1);
	    	    printf("UNLOCK 1 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("this shouldn't happen 1\n");
	    	}
	    } else if (contains(running_process->unlock_2, cpu_pc, 4) == 1) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	if (map->proc != NULL && map->proc == running_process) {
	    	    release_lock(map->lock_2);
	    	    unlock_and_release_waiting_procs(map->lock_2);
	    	    printf("UNLOCK 2 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("this shouldn't happen 2\n");
	    	}
	    } else if (contains(running_process->trylock_1, cpu_pc, 4)) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	int attempt = try_lock(map->lock_1, running_process);
	    	if (attempt == 0) {
	    	    printf("TRY LOCK 1 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("FAILED TRY LOCK 1 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
		}
	    } else if (contains(running_process->trylock_2, cpu_pc, 4)) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	int attempt = try_lock(map->lock_2, running_process);
	    	if (attempt == 0) {
	    	    printf("TRY LOCK 2 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("FAILED TRY LOCK 2 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
		}
	    } else if (contains(running_process->try_unlock_1, cpu_pc, 4)) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	if (map->proc == running_process) {
	    	    release_lock(map->lock_1);
	    	    unlock_and_release_waiting_procs(map->lock_1);
	    	    printf("TRY UNLOCK 1 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("this shouldn't happen 1\n");
	    	}

	    } else if (contains(running_process->try_unlock_2, cpu_pc, 4)) {
	    	proc_to_lock_map_p map = search_list_for_pcb(list_of_locks, running_process);
	    	if (map->proc == running_process) {
	    	    release_lock(map->lock_2);
	    	    unlock_and_release_waiting_procs(map->lock_2);
	    	    printf("TRY UNLOCK 2 proc pid  - %u - pc: %u \n", running_process->pid, cpu_pc);
	    	} else {
	    	    printf("this shouldn't happen 2\n");
	    	}
	    }
        break;
	case PROD:
	    if (running_process != NULL && running_process->proc_type == PROD) {
		if (contains(running_process->prod_cons_lock, cpu_pc + 1, 4) == 1) {
		    int check = lock(prod_cons_locks[running_process->prod_cons_id], running_process); 
		    if (check == 1) {
			lock_trap(NULL);
			break;
		    }
		    
		} else if (running_process != NULL && contains(running_process->prod_cons_lock, cpu_pc, 4) == 1) {
		    
		    if (prod_cons_globals[running_process->prod_cons_id][1] == 1) {
			cond_variable_wait(prod_cons_locks[running_process->prod_cons_id], 
					   prod_cons_cond_vars[running_process->prod_cons_id][1], running_process); // wait for the read
			printf("PID %u requested condition wait on cond %u with mutex %u\n", running_process->pid, 
				running_process->prod_cons_id, running_process->prod_cons_id);
			unlock_and_release_waiting_procs(prod_cons_locks[running_process->prod_cons_id]);
			prod_cons_trap();
		    } else {
			prod_cons_globals[running_process->prod_cons_id][0] += 1;
			prod_cons_globals[running_process->prod_cons_id][1] = 1;
			cond_variable_signal(prod_cons_cond_vars[running_process->prod_cons_id][0], running_process,
					     prod_cons_locks[running_process->prod_cons_id], ready_queue); // signal that it was incremented
			printf("PID %u sent signal on cond %u\n", running_process->pid, running_process->prod_cons_id);
			
			printf("Producer pid %u incremented variable: %i \n", running_process->pid,
			       prod_cons_globals[running_process->prod_cons_id][0]);
		    }
		    
		} else if (contains(running_process->prod_cons_lock, cpu_pc - 1, 4) == 1) {
		    release_lock(prod_cons_locks[running_process->prod_cons_id]);
		    unlock_and_release_waiting_procs(prod_cons_locks[running_process->prod_cons_id]);
		}
	    }
	case CONS:
	    if (running_process != NULL && running_process->proc_type == CONS) {
		if (contains(running_process->prod_cons_lock, cpu_pc + 1, 4) == 1) {
		    int check = lock(prod_cons_locks[running_process->prod_cons_id], running_process); 
		    if (check == 1) {
			lock_trap(NULL);
			break;
		    }
		    
		} else if (running_process != NULL && contains(running_process->prod_cons_lock, cpu_pc, 4) == 1) {
		    if (prod_cons_globals[running_process->prod_cons_id][1] == 0) {
			cond_variable_wait(prod_cons_locks[running_process->prod_cons_id],
					   prod_cons_cond_vars[running_process->prod_cons_id][0], running_process); // wait for the increment
			printf("PID %u requested condition wait on cond %u with mutex %u\n", running_process->pid, 
				running_process->prod_cons_id, running_process->prod_cons_id);
			unlock_and_release_waiting_procs(prod_cons_locks[running_process->prod_cons_id]);
			prod_cons_trap();
		    } else {
			printf("Consumer pid %u read variable: %i \n", running_process->pid, 
			       prod_cons_globals[running_process->prod_cons_id][0]);
			prod_cons_globals[running_process->prod_cons_id][1] = 0;
			cond_variable_signal(prod_cons_cond_vars[running_process->prod_cons_id][1], running_process, 
					     prod_cons_locks[running_process->prod_cons_id], ready_queue); // signal that it was read 
			printf("PID %u sent signal on cond %u\n", running_process->pid, running_process->prod_cons_id);
		    }
		} else if (contains(running_process->prod_cons_lock, cpu_pc - 1, 4) == 1) {
		    release_lock(prod_cons_locks[running_process->prod_cons_id]);
		    unlock_and_release_waiting_procs(prod_cons_locks[running_process->prod_cons_id]);
		}
	    }
	case IO:
	    i = test_io_trap();
	    if (i) {
	    	i--;
	    	printf("EVENT: IO Trap Called for PID %u on IO Device %u\n", running_process->pid, i);
	    	trap_io(i);
	    }
	    break;
	default:
	    break;
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

int contains(unsigned int arr[], unsigned int num, int arr_size) {
    int i;
    for (i = 0; i < 4; i++) {
	if (arr[i] == num) {
	    return 1;
	}
    }
    return 0;
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
void *io_interrupt(unsigned int * io_device) {
    for (;;) {

        PCB_p done_pcb;

	pthread_mutex_lock(&io_lock);
        if (program_executing == 0 && q_is_empty(io_queues[*io_device])) {
	    pthread_mutex_unlock(&io_lock);
	    break;
	}

        
        if (q_is_empty(io_queues[*io_device])) {
	    //pthread_cond_wait(&io_cond, &io_lock);
            if (*io_device == 0) {
                pthread_cond_wait(&io_cond_1, &io_lock);
            } else {
                pthread_cond_wait(&io_cond_2, &io_lock);
            }
        } else {
            struct timespec s;
            s.tv_nsec = (rand() % 1000) + 1;
            nanosleep(NULL, &s);
        }

	// mutex global thread lock
        pthread_mutex_lock(&timer_lock);

	// lock
	// raise flag
	pthread_mutex_lock(&io_init_lock);
	initialized_io = 1;
	pthread_mutex_unlock(&io_init_lock);
	// unlock

	// service on wake up
	done_pcb = q_dequeue(io_queues[*io_device]);
	if (done_pcb != NULL) {
	    /* Increment its PC by 1 to prevent it from going back into IO immediately. */
	    done_pcb->context->pc++;
	    PCB_assign_state(done_pcb, STATE_READY);
	    pq_enqueue(ready_queue, done_pcb);

	    printf("PID %u ready\n", done_pcb->pid);
	    scheduler(INT_IO);
	}

	// lock
	// lower flag
	// unlock
	pthread_mutex_lock(&io_init_lock);
	initialized_io = 0;
	pthread_mutex_unlock(&io_init_lock);


	// release thread mutex
        pthread_mutex_unlock(&timer_lock);

	// release IO specific mutex
	pthread_mutex_unlock(&io_lock);

	// signal to io
	pthread_cond_signal(&io_cond);

	// signal to standard thread!
	// pthread_cond_signal(&timer_cond);


    }
}

/*
 * Determines whether it is time to fire the time interrupt or not.
 */
void *timer() {
    for (;;){
        struct timespec timersleep;
        timersleep.tv_nsec = TIMER_SLEEP;
        nanosleep(0, &timersleep);

        pthread_mutex_lock(&timer_init_lock);
        initialized_cond = 1;
        pthread_mutex_unlock(&timer_init_lock);

        pthread_mutex_lock(&timer_lock);
        
        //set flag to denote timer has started

        printf("EVENT: Timer Interrupt\n");
        print_on_event();

        pseudo_time_interrupt();
        
        //unset flag

        pthread_mutex_lock(&timer_init_lock);
        initialized_cond = 0;
        pthread_mutex_unlock(&timer_init_lock);


        pthread_mutex_unlock(&timer_lock);

        pthread_cond_broadcast(&timer_cond); // broadcast to io dev

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

    // critical section
    // io/timer cannot actually happen in this section as the lock is acquired at this point
    
    running_process->state = STATE_BLOCKED;
    q_enqueue(io_queues[io_device], running_process);
    io_queue_timers[io_device] = quantum_times[running_process->priority] + IO_DELAY_BASE + rand() % IO_DELAY_MOD;
    running_process->context->pc = cpu_pc;
    running_process = NULL;
    print_on_event();

    // after this section check if another thread should take over
    //lock_thread_by_priority(TRAP_IO);
    printf("%d the io device # \n", io_device);

    scheduler(TRAP_IO);

    if (io_device == 0) {
        pthread_cond_signal(&io_cond_1);
    } else {
        pthread_cond_signal(&io_cond_2);
    }
}

/*
 * Tests if the running process should call an IO trap.
 * Returns 1 if IO set 1, 2 if IO set 2.
 */
int test_io_trap() {
    int i;
    if (running_process != NULL) {
        for (i = 0; i < NUM_IO_TRAPS; i++) {
            if (running_process->io_1_traps[i] == cpu_pc) {
                return 1;
            } else if (running_process->io_2_traps[i] == cpu_pc) {
                return 2;
	    }
        }
    }
    return 0;
}


/*
 * Trap for termination.
 * Pre: The running_process must not be NULL.
 */
void trap_terminate() {
    
    switch (running_process->proc_type) { 
    case 0: // IO case
	count_io_procs--;
	break;
    case 1: // computations case
	count_comp_procs--;
	break;
    default:
	return;
    }

    running_process->state = STATE_TERMINATED;
    running_process->termination_time = time(NULL);
    q_enqueue(zombie_queue, running_process);
    running_process = NULL;
    count_terminated++;
    scheduler(INT_TERMINATE);
}

/*
 * The scheduler.
 */
void scheduler(enum interrupt_type type) {
    /* Both values initialized later for speed purposes. */
    PCB_p new_process;
    PCB_p zombie_cleanup;
    PCB_p done_pcb;

    // The idea behind calling lock_thread_by_priority is to defer to a thread 
    // with higher priority.
    // Each block of code in the scheduler is a critical section.
    // After each critical section, give a chance for an interrupt by checking
    // for a signal sent by either a timer or by the io device

    lock_thread_by_priority(type);

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
    
    lock_thread_by_priority(type);
    
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
    
    
    lock_thread_by_priority(type);


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
    
    lock_thread_by_priority(type);


    if (running_process == NULL) {
        dispatcher();
    }
    
    lock_thread_by_priority(type);

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

void lock_thread_by_priority(enum interrupt_type type) {

    // timer interrupt waits for no thread!
    if (type != INT_TIME) {
	
	//acquire lock

	// need to make this non blocking otherwise thread will wait too long
	while (pthread_mutex_trylock(&timer_init_lock) == 1) {
	    while (initialized_cond == 1) {
		pthread_mutex_unlock(&timer_lock);
		pthread_cond_wait(&timer_cond, &timer_init_lock);
		pthread_mutex_lock(&timer_lock);
	    }
	}
	//pthread_mutex_lock(&timer_init_lock);
	pthread_mutex_unlock(&timer_init_lock);

	// need to order io_int -> io_trap
	// acquire io_lock
	// checkcond... sleep if need be
	// release io_lock

	// this block forces the trap routine to defer to the io threads
	if (type == TRAP_IO) {
	    while(pthread_mutex_trylock(&io_init_lock) == 1) {
		while (initialized_io == 1) {
		    pthread_mutex_unlock(&timer_lock);
		    pthread_cond_wait(&io_cond, &io_init_lock);
		    pthread_mutex_lock(&timer_lock);
		}
	    }
	    pthread_mutex_unlock(&io_init_lock);
	}
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
	deadlock_check_counter++;
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


    // init io threads... 
    one = malloc(sizeof(unsigned int));
    *one = 0;
    pthread_create(&io_thread_1, NULL, io_interrupt, (void *) one);
    two = malloc(sizeof(unsigned int));
    *two = 1;
    pthread_create(&io_thread_2, NULL, io_interrupt, (void *) two);

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
    	Lock_p lock_1;
    	Lock_p lock_2;
            /*
             * Randomly decide if one process will be not terminate or not.
             */
    	lottery = rand() % 1000;
    	int type = rand() % NUM_TYPE_PROCS;
    	switch (type) {
    	case 0: //IO CASE
    	    if (count_io_procs < MAX_IO_PROCS) {
    		io_total++;
    	    	new_pcb = make_pcb();
    	    	new_pcb->proc_type = IO;
    	    	count_io_procs++;
    	    	if (lottery <= 5) {
    	    	    new_pcb->terminate = 0;
    	    	}
    	    	q_enqueue(new_queue, new_pcb);
    	    }
    	    break;
    	case 1: // computations case
    	    if (count_comp_procs < MAX_INTENSIVE_PROCS) {
    		intensive_total++;
    	    	new_pcb = make_pcb();
    	    	new_pcb->proc_type = INTENSIVE;
    	    	count_comp_procs++;
    	    	if (lottery <= 5) {
    	    	    new_pcb->terminate = 0;
    	    	}
    	    	q_enqueue(new_queue, new_pcb);
    	    }
    	    break;
    	case 2: // mutex case
    	    if (count_mutex_procs < MAX_MUTEX_PROCS) {
    		lock_1 = lock_constructor();
    	    	lock_2 = lock_constructor();
    	    	mutex_total += 2;
    		count_mutex_procs += 2;

    	    	new_pcb = make_pcb();
    	    	new_pcb->terminate = 0;
    	    	if (new_pcb == NULL) break;
    	    	proc_to_lock_map_p new_map_1 = proc_map_constructor(lock_1, lock_2, new_pcb);
    	    	proc_map_list_add(list_of_locks, new_map_1);
    	    	new_pcb->proc_type = MUTEX;
    	    	q_enqueue(new_queue, new_pcb);

    	    	new_pcb = make_pcb();
    	    	new_pcb->terminate = 0;

    	    	if (new_pcb == NULL) break;
    	    	proc_to_lock_map_p new_map_2;
    	    	if (CREATE_DEADLOCK_TRUE == 0) {
    	    	    new_map_2 = proc_map_constructor(lock_1, lock_2, new_pcb);
    	    	} else {
    	    	    new_map_2 = proc_map_constructor(lock_2, lock_1, new_pcb);
    	    	}
    	    	proc_map_list_add(list_of_locks, new_map_2);
    	    	new_pcb->proc_type = MUTEX;
    	    	q_enqueue(new_queue, new_pcb);
    	    	break;

    	    }
    	case 3: // prod/consumer proc
    	    if (count_prod_cons_procs < MAX_PROD_CONS_PROC_PAIRS) { 
                    // its a prod 
                    //c_Variable_p empty, fill; // used for prod/cons problem
    	    	new_pcb = make_pcb();
    	    	new_pcb->proc_type = PROD;
    	    	new_pcb->prod_cons_id = count_prod_cons_procs;
    	    	prod_cons_cond_vars[count_prod_cons_procs][0] = cond_variable_constructor();
    	    	prod_cons_cond_vars[count_prod_cons_procs][1] = cond_variable_constructor();
    	    	prod_cons_locks[count_prod_cons_procs] = lock_constructor();
    	    	q_enqueue(new_queue, new_pcb);
    	    	// its a cons
    	    	new_pcb = make_pcb();
    	    	new_pcb->proc_type = CONS;
    	    	new_pcb->prod_cons_id = count_prod_cons_procs;
    	    	count_prod_cons_procs++;
    	    	q_enqueue(new_queue, new_pcb);
    	    }
    	    break;
    	default:
    	    break;
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
        my_pcb->max_pc = MIN_MAX_PC_VAL + (rand() % MAX_PC_MODULO);
        /* Start the PC at some value < max_pc for testing. */
        my_pcb->context->pc = 0;
        /*
         * Set the number of runs before termination to a random number between
         * MIN_NUM_BEFORE_TERM and MIN_NUM_BEFORE_TERM + RANDOM_NUM_BEFORE_TERM + 1
         */
        my_pcb->terminate = MIN_NUM_BEFORE_TERM + rand() % RANDOM_NUM_BEFORE_TERM;

	i = 0;
	while (i < NUM_IO_TRAPS) {
	    unsigned int first = rand() % (my_pcb->max_pc/NUM_IO_TRAPS);
	    unsigned int second = rand() % (my_pcb->max_pc/NUM_IO_TRAPS);

	    my_pcb->io_1_traps[i] = first;
	    my_pcb->io_2_traps[i] = second;
            /* If we're past the first io trap, add the previous value to this one. */
            if (i > 0) {
                my_pcb->io_1_traps[i] = my_pcb->io_1_traps[i] + my_pcb->io_1_traps[i-1];
                my_pcb->io_2_traps[i] = my_pcb->io_2_traps[i] + my_pcb->io_2_traps[i-1];
            }

	    if (exists(my_pcb->io_2_traps[i], my_pcb->io_1_traps, i, my_pcb) == 1 &&
		exists(my_pcb->io_1_traps[i], my_pcb->io_2_traps, i, my_pcb) == 1) {
	    } else {
		i++;
	    }
        }
    }
    return my_pcb;
}

int exists(unsigned int check, unsigned int arr[], int size, PCB_p proc) {
    int i;
    for (i = 0; i < 4; i++) {
	if (exists_in_range(proc->lock_1[i], proc->unlock_1[i], check) == 1 ||
	    exists_in_range(proc->trylock_1[i], proc->try_unlock_1[i], check) == 1 ||
	    exists_in_range(proc->prod_cons_lock[i]-1, proc->prod_cons_lock[i]+1, check) ==1) {
	    return 1;
	}
    }

    if (contains(arr, check, size) == 1) return 1;

    return 0;
}

int exists_in_range(unsigned int base, unsigned int bound, unsigned int check) {
    return check >= base && check <= bound;
}



/*
 * Checks for deadlock
 */
int deadlock_monitor() {
    proc_node_p currnode = list_of_locks->head;
    int sequential_check = 0;
    while (currnode != NULL) {
	if (sequential_check != 0) { // used to avoid double-reporting deadlock on the second proc in pair
	    sequential_check = 0;
	    currnode = currnode->next;
	    continue;
	} else {
	    PCB_p mutproc1 = currnode->map->proc;
	    sequential_check = 1;
	    Lock_p currlock1 = currnode->map->lock_1; 
	    Lock_p currlock2 = currnode->map->lock_2;
	    if ((currlock1->current_proc == NULL || currlock2->current_proc == NULL)
	    	|| (currlock1->current_proc == mutproc1 && currlock2->current_proc == mutproc1)) {
	        currnode = currnode->next;
	        continue;
	    } else { 
	       if ((currlock1->current_proc == mutproc1 && q_peek(currlock2->waiting_procs) == mutproc1) // if proc has lock 1 and is waiting on lock 2,
	    	   || (q_peek(currlock1->waiting_procs) == mutproc1 && currlock2->current_proc == mutproc1)) { // or if it has lock 2 and is waiting on 1
	           printf("Deadlock detected on processes PID%u and PID%u\n", mutproc1->pid, mutproc1->pid + 1);
	           currnode = currnode->next;
		       deadlock_flag=1;
		      continue;
	       } else {
	           currnode = currnode->next;
	           continue;
	       }
	    }
	}
    }
    return 0;
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
    if (deadlock_flag == -1) {
	printf("No deadlock found\n");
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
    free(one);
    free(two);

}

void lock_trap(Lock_p lock) {
    running_process->context->pc = cpu_pc - 1;
    running_process->state = STATE_BLOCKED;
    running_process = NULL;
    scheduler(TRAP_IO);
}

void unlock_and_release_waiting_procs(Lock_p lock) {
    FIFOq_p q = lock->waiting_procs;
    while (q->size > 0) {
	PCB_p proc = q_dequeue(q);
	proc->state = STATE_READY;
	pq_enqueue(ready_queue, proc);
    }
}

void prod_cons_trap() {
    running_process->context->pc = cpu_pc - 1;
    running_process->state = STATE_BLOCKED;
    running_process = NULL;
    scheduler(TRAP_PROD_CONS);
}
