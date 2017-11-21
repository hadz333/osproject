
objects= cpu_loop.c priority_queue.c fifo_queue.c pcb.c mutex_lock.c cond_variable.c

cpu_loop:
	gcc -pthread -o cpu_loop $(objects)

debug:
	gcc -pthread -o cpu_loop -ggdb -Wall cpu_loop.c priority_queue.c fifo_queue.c pcb.c

clean:
	rm cpu_loop && make cpu_loop
