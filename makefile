all:
	gcc -pthread -o cpu_loop cpu_loop.c priority_queue.c fifo_queue.c pcb.c

debug:
	gcc -pthread -o cpu_loop -ggdb -Wall cpu_loop.c priority_queue.c fifo_queue.c pcb.c
