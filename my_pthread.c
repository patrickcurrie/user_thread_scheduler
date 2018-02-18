// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

/* Globals */
scheduler * SCHEDULER;
int NUMBER_LEVELS;
int NUMBER_LOCKS;
int TIME_SLICE;
int STACK_SIZE = 8192; // 8192 kbytes is default stack size for CentOS

/* Static internal functions */

/*
Handles argument passing to the function run by a thread, preperation for the
thread to run, and cleanup after the thread finishes.
*/
static void thread_function_wrapper(tcb *tcb_node, void *(*function) (void *), void *arg) {
	SCHEDULER->current_tcb = tcb_node;
	tcb_node->state = RUNNING;
	tcb_node->start_time = current_time();
	tcb_node->return_value = (*function)(arg);
	tcb_node->state = TERMINATED;
	SCHEDULER->current_tcb = NULL;
	scheduler_maintenance(); // Clean terminated thread from SCHEDULER->multi_level_priority_queue.
}

/*
Enqueue's a tcb to a given level of the multi-level priority queue. Sets
the state of the thread tcb to READY.
*/
static int schedule_thread(tcb * tcb_node, int priority) {
	if (priority < 0 || priority > NUMBER_LEVELS - 1) {
		return -1; // Error invalid priority.
	}
	tcb_node->state = READY;
	tcb_node->priority = priority;
	enqueue(&(SCHEDULER->multi_level_priority_queue[priority]), tcb_node);
	return 0;
}

/* Queue Functions */

/* Initializes a queue */
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
	if (q->size == 0) {
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
tcb *dequeue(queue * q) {
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
		q->head = q->head->next_tcb;
	}
	tmp->next_tcb = NULL;
	q->size--;
	return tmp;
}

tcb *peek(queue * q) {
	return q->head;
}

/* Get current time */
struct timeval current_time() {
	struct timeval *tv;
	gettimeofday(tv, NULL);
	return *tv;
}

/* Initialize scheduler */
void init_scheduler() {
	SCHEDULER = malloc(sizeof(scheduler));
	SCHEDULER->multi_level_priority_queue = malloc(sizeof(queue) * NUMBER_LEVELS);
	for (int i = 0; i < NUMBER_LEVELS; i++) {
		queue_init(&(SCHEDULER->multi_level_priority_queue[i]));
	}

	SCHEDULER->wait_queues = NULL; // New wait_queue is malloced in my my_pthread_mutex_init function.
	SCHEDULER->scheduler_tcb = malloc(sizeof(tcb)); // Set when context is swapped out of scheduler context.
	SCHEDULER->current_tcb = NULL; // New tcb malloced in my_pthread_create function.
	SCHEDULER->priority_time_slices = malloc(sizeof(int) * NUMBER_LEVELS);
	for (int i = 0; i < NUMBER_LEVELS; i++) {
		SCHEDULER->priority_time_slices[i] = TIME_SLICE * (i + 1);
	}

	signal(SIGALRM, scheduler_maintenance);
	alarm(1); // Maybe this should be settimer? Need a way to fire SIGALRM signal every n seconds.
}

/*
Maintenance done on the multi-level priority queue to handle the SIGALRM signal.

Responsible for:
- Deleting threads with TERMINATED state from multi-level priotity queue (free tcb and adjust queue).
- Promoting and demoting threads in multi-level priority queue.
- Check if current running tcb (SCHEDULER->current_tcb) has used up its time slice, swap context and adjust accordingly if so.
*/
void scheduler_maintenance() {
	printf("Test\n");
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function) (void *), void *arg) {
	// Create new tcb for thread.
	// Get current context.
	tcb *tcb_node = malloc(sizeof(tcb));
	tcb_node->tid = *thread;
	if (getcontext(&(tcb_node->context)) != 0) {
		return -1; // Error getthing context.
	}
	// Configure context stack.
	tcb_node->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	tcb_node->context.uc_stack.ss_flags = 0;
	tcb_node->context.uc_stack.ss_size = STACK_SIZE;
	makecontext(&(tcb_node->context), (void *) thread_function_wrapper, 3, tcb_node, function, arg);
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	int current_priority = SCHEDULER->current_tcb->priority;
	tcb *tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
	tcb_node->state = READY;
	schedule_thread(tcb_node, tcb_node->priority);
	SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority]));
	// Swap context to new SCHEDULER->current_tcb->context, store current context to &(SCHEDULER->scheduler_tcb->context)
	SCHEDULER->current_tcb->state = RUNNING;
	tcb_node->last_yield_time = current_time();
	swapcontext(&(tcb_node->context), &(SCHEDULER->current_tcb->context));
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
	if (NUMBER_LOCKS == 0) {
		SCHEDULER->wait_queues = malloc(sizeof(queue));
		NUMBER_LOCKS++;
	} else {
		SCHEDULER->wait_queues = realloc(SCHEDULER->wait_queues, sizeof(queue) * ++NUMBER_LOCKS);
	}
	my_pthread_mutex_t *mutex = malloc(sizeof(my_pthread_mutex_t));
	mutex->lock_owner = NULL;
	mutex->lock_wait_queue = &(SCHEDULER->wait_queues[NUMBER_LOCKS - 1]); // This is this locks wait queue.

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

int main() {
	// Test code here
	init_scheduler();
	sleep(10);
	return 0;
}
