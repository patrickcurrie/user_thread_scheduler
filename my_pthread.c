// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

/* Globals */
scheduler * SCHEDULER;
int NUMBER_LEVELS;
int NUMBER_LOCKS = 0;
int TIME_SLICE;
int HAS_RUN=0;
int MAINTAIN;
int STACK_SIZE = 8192; // 8192 kbytes is default stack size for CentOS

/* Static internal functions */

/*
Handles argument passing to the function run by a thread, preperation for the
thread to run, and cleanup after the thread finishes.
*/
static void thread_function_wrapper(tcb *tcb_node, void *(*function) (void *), void *arg) {
	SCHEDULER->current_tcb = tcb_node;
	tcb_node->state = RUNNING;
	tcb_node->initial_start_time = current_time();
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
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
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
        execute();
}

/*a function that do nothing but blocks signals */
void block(){}

/*Wait until there is some thread can be scheduled*/
void execute(){
        if(SCHEDULER->current_tcb==NULL){
                int indicate = 0;
                for(int i=0; i<NUMBER_LEVELS;i++){
                        if(SCHEDULER->multi_level_priority_queue[i].size>0){
                                SCHEDULER->current_tcb = SCHEDULER->multi_level_priority_queue[i].head;
                                indicate=1;
                                setcontext(SCHEDULER->current_tcb->context);
                        }
                }
                if(indicate==0){
                        execute();
                }
        }
        struct itimerval value_yield, value_maintain, ovalue; //(1)
        signal(SIGALRM, my_pthread_yield);
        signal(SIGVTALRM,scheduler_maintenance);
        value_yield.it_value.tv_sec = 0;
        value_yield.it_value.tv_usec = SCHEDULER->priority_time_slices[SCHEDULER->current_tcb->priority];
        value_yield.it_interval.tv_sec = 0;
        value_yield.it_interval.tv_usec = SCHEDULER->priority_time_slices[SCHEDULER->current_tcb->priority];
        setitimer(ITIMER_REAL, &value_yield, &ovalue); //(2)
        value_maintain.it_value.tv_sec = 0;
        value_maintain.it_value.tv_usec = MAINTAIN;
        value_maintain.it_interval.tv_sec = 0;
        value_maintain.it_interval.tv_usec = MAINTAIN;
        setitimer(ITIMER_VIRTUAL, &value_maintain, &ovalue);
        for(;;);

}

/*A helper function for maintain to compare time*/
/* return 1 if the gap between start and end is larger than gap
 * return 0 if the gap between start and end is the same as the gap
 * return -1 if the gap between start and end is smaller than the gap
 * */
/*PS: gap's unit is in microsecond*/
int time_compare(struct timeval start, struct timeval end, int gap){
    int gap_second = 0;
    int gap_microsecond = 0;
    int start_sec = start.tv_sec;
    int start_micro = start.tv_usec;
    int end_sec = end.tv_sec;
    int end_micro = end.tv_usec;
    if(gap>999999){
        gap_second = gap-(gap%1000000);
        gap_microsecond = gap%1000000;
    }else{
        gap_microsecond = gap;
    }
    if(end_sec-start_sec>gap_second){
        return 1;
    }else if(end_sec-start_sec==gap_second){
        if(end_micro-start_micro>gap_microsecond){
            return 1;
        }else if(end_micro-start_micro==gap_microsecond){
            return 0;
        }else{
            return -1;
        }
    }else{
        return -1;
    }

}


/* a helper function for scheduler_maintenance.
 * Check to see if the thread need to be promote to a higher queue or a lower
 * return 1 to promote to a higher queue
 * return 0 to stay in the original queue
 * return -1 to degrade to a lower queue
 * */
int promotion(tcb* tcb_node){
        int priority  = tcb_node->priority;
        struct timeval start_time = tcb_node->initial_start_time;
        struct timeval end_time = tcb_node->last_yield_time;
        int time = SCHEDULER->priority_time_slices[priority];
        if(time_compare(start_time,end_time,time)!=-1){
                return -1;
        }else{
                return 0;
        }
}

/* a helper function for maintenance*/
void remove_tcb(queue* current_queue, tcb* prev, tcb* current){
    if(current_queue->size==1){
        current_queue->tail=NULL;
        current_queue->tail=NULL;
        current=NULL;
        prev = current;
    }
    if (current!=prev) { //check if this is the head of the queue
        current = current->next_tcb;
        prev = current;
        current_queue->head = current;
    }else if(current==current_queue->tail){//check if this is the tail of the queue
        current=NULL;
        prev->next_tcb=NULL;
        current_queue->tail = prev;
    }else{
        current = current->next_tcb;
        prev->next_tcb = current;
    }
}
/*
Maintenance done on the multi-level priority queue to handle the SIGALRM signal.

Responsible for:
- Deleting threads with TERMINATED state from multi-level priotity queue (free tcb and adjust queue).
- Promoting and demoting threads in multi-level priority queue.
- Check if current running tcb (SCHEDULER->current_tcb) has used up its time slice, swap context and adjust accordingly if so.
*/
void scheduler_maintenance() {
        //block signal
        signal(SIGALRM, block);
        signal(SIGVTALRM, block);
    //first check if the thread used up its time slice
	int p =  SCHEDULER->current_tcb->priority;
     int time_slice = SCHEDULER->priority_time_slices[p];
    tcb* current_running = SCHEDULER->current_tcb;
    if(time_compare(current_time(),current_running->last_yield_time,time_slice)!=-1){
        my_pthread_yield();
    }
    //loop through all the queue and check for deletion and promotion
    for(int i=0; i<NUMBER_LEVELS;i++){
        int size = SCHEDULER->multi_level_priority_queue[i].size;
        queue* current_queue = &SCHEDULER->multi_level_priority_queue[i];
        tcb* current = current_queue->head;
        tcb* prev = current_queue->head;
        while(current!=NULL){
            //first check if the state of any tcb is terminated, if yes release the recourse
            if(current->state==TERMINATED){
                tcb* tmp = current;
                remove_tcb(current_queue,prev,current);
                current_queue->size--;
                free(tmp);
            }else if (promotion(current)==1){ //to a higher level queue
                if(p!=0){
                    tcb* tmp = current;
                    remove_tcb(current_queue,prev,current);
                    current_queue->size--;
                    enqueue(&SCHEDULER->multi_level_priority_queue[p+1],tmp);
                    SCHEDULER->multi_level_priority_queue[p+1].size++;
                    tmp->priority++;
                }
            }else if(promotion(current)==-1){
                if(p!=NUMBER_LEVELS-1){
                    tcb* tmp = current;
                    remove_tcb(current_queue,prev,current);
                    current_queue->size--;
                    enqueue(&SCHEDULER->multi_level_priority_queue[p-1],tmp);
                    SCHEDULER->multi_level_priority_queue[p-1].size++;
                    tmp->priority--;
                }
            }else{
                prev = current;
                current = current->next_tcb;
            }
        }

    }
        signal(SIGALRM, my_pthread_yield);
        signal(SIGVTALRM,scheduler_maintenance);
        return;
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
        schedule_thread(tcb_node, 0);
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
        //block signal
        signal(SIGALRM, block);
        signal(SIGVTALRM, block);
	int current_priority = SCHEDULER->current_tcb->priority;
	tcb *tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
	if (tcb_node->state != TERMINATED) {
		tcb_node->state = READY;
	}
	schedule_thread(tcb_node, tcb_node->priority);
    //check to see if we need to move on to the next queue
    if(HAS_RUN>=SCHEDULER->multi_level_priority_queue[current_priority].size){
        HAS_RUN=0; //running a new queue set the counter to 0
        if(current_priority==NUMBER_LEVELS-1){// this is the lowest priority
            SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[0])); //run the highest priority queue
        }else{
            SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority+1])); //run the next priority queue
        }
    }else{
        SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority])); //stay in the same queue
    }
	// Swap context to new SCHEDULER->current_tcb->context, store current context to &(SCHEDULER->scheduler_tcb->context)
	if (SCHEDULER->current_tcb->state == TERMINATED) { // Don't run context if TERMINATED
		my_pthread_yield();
		return 0;
	}
	SCHEDULER->current_tcb->state = RUNNING;
	tcb_node->last_yield_time = current_time();
        SCHEDULER->current_tcb->recent_start_time = current_time();
	swapcontext(&(tcb_node->context), &(SCHEDULER->current_tcb->context));
    	HAS_RUN++;
        signal(SIGALRM, my_pthread_yield);
        signal(SIGVTALRM,scheduler_maintenance);
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
