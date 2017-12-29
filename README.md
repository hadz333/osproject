# TCSS 422 - Computer Operating Systems - Process Scheduler Simulation

## Summary
This program was the culminating project for University of Washington Tacoma's TCSS 422 - Computer Operating Systems course. Our group consisted of Dakota Crane, Dino Hadzic, and Tyler Stinson. The project's objective was to design and implement a simulated OS process scheduler capable of handling concurrent processes, interprocess communications, and asynchronous I/O devices.

There were several specified process types we were to include, as well as simulated features such as a termination scheme, IO trap vectors, and a deadlock monitor. Additionally, we were to roll our own mutex lock and condition variable ADTs with attendant functions for use with our two concurrency-based process types.

## Processes
There were four types of processes implemented:

* Computationally intensive: These processes are essentially 'filler' procs, insofar as they do not interact with our mutex structures or our IO devices and just take up runtime.

* IO: Using the IO thread, these devices periodically engage an IO trap routine, placing the process into a blocked state before the scheduler switched to a new running process. After some time, an IO return interrupt occurs, necessitating the completion of the proc's IO request and returning it to a ready state.

* Mutual resource users: These processes are one of two concurrency-based proc types. Our mutual resource users are created in pairs and attempt to access a global variable shared between them. This is faciliated using our homegrown mutex locks. By default, these locks are held and released in the correct order to avoid deadlock; by changing a flag, deadlock can be achieved, which is watched for with the deadlock monitor function. 

* Producer-consumer pairs: These procs work together, using our homegrown condition variable mutexes, to work on a global variable per pair. If the variable has been read, the producer increments the variable and signals the consumer; if the consumer is signalled, it will read the variable and signal that it has been read. 

