# Operating Systems - Final Project

## TODO Overview
This is in no way conclusive so far, please add and make better

### CPU
Tests trylock(timer lock)
tests trylock(IO lock)
pc++

### Timer
sleeps(some ms)
awake and acquire timer lock (signal timer interrupt)
calls timer isr

### IO devices
When len(io queue) > 0:
sleep(some ms)
awake and acquire io lock (signals io interrupt)
calls io isr/scheduler
!! Need to be able to deal with timer interrupts within IO interrupts

### Terminate
Essentially per problem 4?

### Sync Structs
Contain pointer to process that owns the struct
Contains FIFO queue to blocked processes waiting for the lock/whatever

### New process types
Will need arrays (like the IO trap arrays in the pcbs in problem 4) for synchronous occurrences (lock, trylock, unlock / wait, signal)

#### Producer-Consumer
Will need producer and consumer processes, as well as put/get functs
Will use wait/signal type sync structs (condition variables
Good example at end of ch 31 (I think that's the ch. need to check)

#### Mutual resource users
Will use two locks shared between process pairs

#### IO Processes
Basically as per problem 4; flesh out later

#### Computationally intensive processes
Basically as per problem 4; flesh out later

### Deadlock monitor
Worry about this later
