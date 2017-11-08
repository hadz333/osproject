objects= cpu_loop.c priority_queue.c fifo_queue.c pcb.c mutex_lock.c cond_variable.c
all: $(objects)
	gcc -o cpu_loop $(objects)

debug:
	gcc -o cpu_loop -ggdb -Wall cpu_loop.c priority_queue.c fifo_queue.c pcb.c
