// name:
// username of iLab:
// iLab Server:
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <ucontext.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	/* add something here */
        my_pthread_t tid;
        ucontext_t context;
        thread_state state;
        int priority;
        struct timeval start_time;
        struct timeval last_yield_time;
        void *return_value;
        tcb *next_tcb;
} tcb;

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures

typedef enum {
        RUNNING, // Currently running.
        READY, // Scheduled (in multi-level priority queue), in line to be run.
        WAITING, // Waiting in wait queue to aquire lock to critical section.
        TERMINATED // Thread finished running or was terminated early.
} thread_state;

typedef struct {
        queue *multi_level_priority_queue;
        queue *wait_queue;
        tcb *scheduler_tcb; // So  we know where to to return to.
        tcb *current_tcb;
        int *priority_time_slices;
} scheduler;

typedef struct {
	tcb *head;
	tcb *tail;
	int size;
} queue;

/* Function Declarations: */

/* Queue Functions */
void queue_init(queue *q);

void enqueue(queue *q, tcb *tcb_node);

tcb * dequeue(queue *q);

/* Get current time */
struct *timeval current_time();

/* Scheduler functions */
void init_scheduler();

void scheduler_maintenance();

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t *attr, void *(*function)(void*), void *arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
