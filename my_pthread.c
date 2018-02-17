// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

/* Globals */
scheduler * SCHEDULER;
int NUMBER_LEVELS;
int TIME_SLICE;
int STACK_SIZE = 8192; // 8192 kbytes is default stack size for CentOS

/* Static internal functions */
static void thread_function_wrapper(tcb *thread_tcb, void *(*function) (void *), void *arg) {
	SCHEDULER->current_tcb = thread_tcb;
	thread_tcb->state = RUNNING;
	thread_tcb->return_value = (*function)(arg);
	thread_tcb->state = TERMINATED;
	SCHEDULER->current_tcb = NULL;
	scheduler_maintenance(); // Clean terminated thread from SCHEDULER->multi_level_priority_queue.
}

static void schedule_thread(tcb * thread_tcb, int priority) {
	
}

/* Queue Functions */
void queue_init(queue * q) {
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
}

/*
If the queue is empty, than the head and tail will point to the same node after
enqueue. If it is not empty, then node will be inserted before the tail of the
queue, and become the new tail.
*/
void enqueue(queue * q, tcb * tcb_node) {
	if (first->size == 0) {
		q->head = tcb_node;
		q->tail = tcb_node;
		q->size++;
	} else {
		q->tail->next_tcb = tcb_node;
		q->tail = tcb_node;
		q->size++;
	}
}

/*
If queue is empty, returns NULL. If queue has onlu 1 node, set tmp to what
head points to, then set head and tail to NULL and return tmp. Else, set tmp to
what head points to, and adjust head.
*/
tcb * dequeue(queue * q) {
	if (q->size == 0) {
		return NULL;
	}
	tcb * tmp;
	if (q->size == 1) {
		tmp = q->head;
		q->head = NULL;
		q->tail = NULL;
	} else {
		tmp = q->head;
		q->head = qt->head->next_tcb;
	}
	tmp->next_tcb = NULL;
	first->size--;
	return tmp;
}

/* Get current time */
struct *timeval current_time() {
	struct *timeval tv;
	gettimeofday(tv, NULL);
	return tv;
}

/* Initialize scheduler */
void init_scheduler() {
	SCHEDULER = malloc(sizeof(scheduler));
	SCHEDULER->multi_level_priority_queue = malloc(sizeof(queue) * NUMBER_LEVELS);
	for (int i = 0; i < NUMBER_LEVELS; i++) {
		queue_init(SCHEDULER->multi_level_priority_queue[i]);
	}

	SCHEDULER->wait_queue = malloc(sizeof(queue));
	queue_init(SCHEDULER->wait_queue);
	SCHEDULER->current_tcb = NULL; // New tcb malloced in my_pthread_create function.
	SCHEDULER->priority_time_slices = malloc(sizeof(int) * NUMBER_LEVELS);
	for (int i = 0; i < NUMBER_LEVELS; i++) {
		SCHEDULER->priority_time_slices[i] = TIME_SLICE * (i + 1);
	}

	signal(SIGALRM, scheduler_maintenance);
}

/*
Maintenance done on the multi-level priority queue to handle the SIGALRM signal.

Responsible for:
- Deleting threads with TERMINATED state from multi-level priotity queue (free tcb and adjust queue).
- Promoting and demoting threads in multi-level priority queue.
*/
void scheduler_maintenance() {

}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function) (void *), void *arg) {
	// Create new tcb for thread
	// Get current context
	tcb *thread_tcb = malloc(sizeof(tcb));
	thread_tcb->tid = thread;
	if (getcontext(thread_tcb->context)) != 0) {
		return -1; // Error getthing context.
	}

	// Use current context for as template for new context
	thread->context->uc_stack.ss_sp = malloc(STACK_SIZE);
	thread->context->uc_stack.ss_flags = 0;
	thread->context->uc_stack.ss_size = STACK_SIZE;
	makecontext(&(thread->context, (void *) thread_function_wrapper, 3, thread_tcb, function, arg);

	// Instruct scheduler to start procedure for scheduling thread.

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};
