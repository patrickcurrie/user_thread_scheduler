// name: Patrick Currie, Ran Sa, Yaowen Zhang;
// username of iLab:
// iLab Server:

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	/* add something here */
} tcb;

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
} my_pthread_mutex_t;

/* define your data structures here: */

/*the node for the queue*/
typedef struct my_pthread_node{
    //the thread stored in the node
    my_pthread_t* value;
    struct my_pthread_node* next;
}tNode;

/*a linked list act like a queue*/
typedef struct my_pthread_queue{
    /* the front and the end of the queue; used for dequeue and enqueue*/
    tNode*  front;
    tNode* end;
    /* size: how may thread are in this queue*/
    int size;
}tQueue;
// Feel free to add your own auxiliary data structures


/* Function Declarations: */

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

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