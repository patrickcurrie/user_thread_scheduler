// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
void print_queue();
/* Globals */
scheduler * SCHEDULER;
int SCHEDULER_INIT = 0;
int NUMBER_LEVELS = 3;
int NUMBER_LOCKS = 0;
int TIME_SLICE = 5000;
int HAS_RUN=0;
int START = 0;
int CYCLE = 0;
int STACK_SIZE = 8192; // 8192 kbytes is default stack size for CentOS
int T_ID = 0;
int first_thread = 0;

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
        tcb_node->next_tcb = NULL;
		q->size++;
	} else {
		q->tail->next_tcb = tcb_node;
		q->tail = tcb_node;
        q->tail->next_tcb=NULL;
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
    printf("init scheduler\n");
	SCHEDULER = malloc(sizeof(scheduler));
	SCHEDULER->multi_level_priority_queue = malloc(sizeof(queue) * NUMBER_LEVELS);
    int i;
	for (i = 0; i < NUMBER_LEVELS; i++) {
		queue_init(&(SCHEDULER->multi_level_priority_queue[i]));
	}

	SCHEDULER->wait_queues = NULL; // New wait_queue is malloced in my my_pthread_mutex_init function.
	SCHEDULER->main_tcb = NULL; // Set only after first call to my_pthread_create funtion.
	SCHEDULER->current_tcb = NULL; // New tcb malloced in my_pthread_create function.
	SCHEDULER->priority_time_slices = malloc(sizeof(int) * NUMBER_LEVELS);
	for (i = 0; i < NUMBER_LEVELS; i++) {
		SCHEDULER->priority_time_slices[i] = TIME_SLICE * (i + 1);
	}
}

void execute(){
        //setup signal
        struct itimerval value_yield,ovalue; //(1)
        signal(SIGALRM, signal_handler);
        value_yield.it_value.tv_sec = 0;
        value_yield.it_value.tv_usec = 25000;
        value_yield.it_interval.tv_sec = 0;
        value_yield.it_interval.tv_usec = 25000;
        setitimer(ITIMER_REAL, &value_yield, &ovalue); //(2)

}


void signal_handler(){
        sigset_t block;
        sigemptyset(&block);
        sigaddset(&block, SIGALRM);
        sigprocmask(SIG_BLOCK, &block, NULL);
        CYCLE++;
        printf("the cycle number is: %d\n", CYCLE);
        if(first_thread == 0){
            return;
        }
        if(START==0){
                START=1;

                SCHEDULER->current_tcb = SCHEDULER->multi_level_priority_queue[0].head;
                SCHEDULER->current_tcb->state = RUNNING;
                SCHEDULER->current_tcb->recent_start_time = current_time();
                setcontext(&SCHEDULER->current_tcb->context);
                HAS_RUN++;
        }

        if(CYCLE == 5){
                CYCLE=0;
                printf("Before maintain: %d\n", SCHEDULER->current_tcb->tid);
                scheduler_maintenance();
                print_queue();
        }
        if(time_compare(SCHEDULER->current_tcb->recent_start_time,current_time(),SCHEDULER->priority_time_slices[SCHEDULER->current_tcb->priority])!=-1){ //check if it need to yield
                printf("Before yield: %d\n", SCHEDULER->current_tcb->tid);
                my_pthread_yield();
                printf("After yield: %d\n", SCHEDULER->current_tcb->tid);
        }
        sigprocmask(SIG_UNBLOCK, &block, NULL);
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
tcb* remove_tcb(queue* current_queue, tcb* prev, tcb* current){
    if(current_queue->size==1){
        printf("The only one\n");
        current_queue->tail=NULL;
        current_queue->head=NULL;
        current=NULL;
        prev = current;
        return current;
    }
    if (current==current_queue->head) { //check if this is the head of the queue
        printf("head\n");
        current = current->next_tcb;
        prev = current;
        current_queue->head = current;
    }else if(current==current_queue->tail){//check if this is the tail of the queue
        printf("tail\n");
        current=NULL;
        prev->next_tcb=NULL;
        current_queue->tail = prev;
    }else{
        printf("middle\n");
        current = current->next_tcb;
        prev->next_tcb = current;
    }
    return current;
}
/*
Maintenance done on the multi-level priority queue to handle the SIGALRM signal.

Responsible for:
- Deleting threads with TERMINATED state from multi-level priotity queue (free tcb and adjust queue).
- Promoting and demoting threads in multi-level priority queue.
- Check if current running tcb (SCHEDULER->current_tcb) has used up its time slice, swap context and adjust accordingly if so.
*/
void scheduler_maintenance() {
        printf("Entered a maintenance cycle\n");
        int p =  0;
        //create an array to store 3 oldest thread
        tcb* oldest[6];
        //set all the 6 slot to 0
        int index=0;
        for(index = 0; index<6; index++){
                oldest[index]=NULL;
        }
        //loop through all the queue and check for deletion and promotion
        int i=0;
        int x = 0;
        printf("Before first for loop\n");
        for(i=0; i<NUMBER_LEVELS;i++){
            printf("In the first for loop\n");
            print_queue();
                p=i;
                int size = SCHEDULER->multi_level_priority_queue[i].size;
                queue* current_queue = &SCHEDULER->multi_level_priority_queue[i];
                printf("size of the queue is:%d\n", current_queue->size);
                tcb* current = current_queue->head;
                tcb* prev = current_queue->head;
                while(current!=NULL){
                        printf("In the first while loop\n");
                        //first check if the state of any tcb is terminated, if yes release the recourse
                        if(current->state==TERMINATED){
                                printf("In Terminated, the tid is: %d\n", current->tid);
                                tcb* tmp = current;
                                current = remove_tcb(current_queue,prev,current);
                                current_queue->size--;
                                free(tmp);
                        }else if(promotion(current)==-1){
                                printf("In Promotion, the tid is: %d\n", current->tid);
                                if(p!=NUMBER_LEVELS-1){
                                        tcb* tmp = current;
                                        current = remove_tcb(current_queue,prev,current);
                                        tmp->next_tcb=NULL;
                                        current_queue->size--;
                                        enqueue(&SCHEDULER->multi_level_priority_queue[p+1],tmp);
                                        SCHEDULER->multi_level_priority_queue[p+1].size++;
                                        tmp->priority--;
                                }else{
                                    prev = current;
                                    current = current->next_tcb;
                                }
                        }else{
                                printf("In Else, the tid is: %d\n", current->tid);
                                prev = current;
                                current = current->next_tcb;
                        }
                }

        }
        printf("Before second for loop\n");
        for(i=0; i<NUMBER_LEVELS;i++){
                p=i;
                int size = SCHEDULER->multi_level_priority_queue[i].size;
                queue* current_queue = &SCHEDULER->multi_level_priority_queue[i];
                tcb* current = current_queue->head;
                tcb* prev = current_queue->head;
                while(current!=NULL){
                        //first check if the state of any tcb is terminated, if yes release the recourse
                        if (oldest[1]==NULL||oldest[3]==NULL||oldest[5]==NULL){ //to a higher level queue
                                if(oldest[1]==NULL){
                                        oldest[0]=prev;
                                        oldest[1] = current;
                                }else if(oldest[3]==NULL){
                                        oldest[2] = prev;
                                        oldest[3] = current;
                                }else{
                                        oldest[4] = prev;
                                        oldest[5] = current;
                                }
                                prev = current;
                                current = current->next_tcb;
                        }else if(timercmp(&SCHEDULER->current_tcb->initial_start_time,&oldest[5]->initial_start_time,<)>0){
                                if(timercmp(&SCHEDULER->current_tcb->initial_start_time,&oldest[3]->initial_start_time,<)>0){
                                        if(timercmp(&SCHEDULER->current_tcb->initial_start_time,&oldest[1]->initial_start_time,<)>0) {
                                                oldest[0] = prev;
                                                oldest[1] = current;
                                        }else{
                                                oldest[2] = prev;
                                                oldest[3] = current;
                                        }
                                }else{
                                        oldest[4] = prev;
                                        oldest[5] = current;
                                }
                                prev = current;
                                current = current->next_tcb;
                        } else{
                                prev = current;
                                current = current->next_tcb;
                        }
                }

        }
        printf("After for loop\n");
        if(oldest[5]!=NULL){
                if(oldest[5]->priority!=0) {
                        tcb *tmp = oldest[5];
                        p = tmp->priority;
                        remove_tcb(&SCHEDULER->multi_level_priority_queue[p], oldest[4], oldest[5]);
                        SCHEDULER->multi_level_priority_queue[p].size--;
                        enqueue(&SCHEDULER->multi_level_priority_queue[p-1], tmp);
                        SCHEDULER->multi_level_priority_queue[p-1].size++;
                        tmp->priority++;
                }
        }
        if(oldest[3]!=NULL){
                if(oldest[3]->priority!=0) {
                        tcb *tmp = oldest[3];
                        p = tmp->priority;
                        remove_tcb(&SCHEDULER->multi_level_priority_queue[p], oldest[2], oldest[3]);
                        SCHEDULER->multi_level_priority_queue[p].size--;
                        enqueue(&SCHEDULER->multi_level_priority_queue[p-1], tmp);
                        SCHEDULER->multi_level_priority_queue[p-1].size++;
                        tmp->priority++;
                }
        }
        if(oldest[1]!=NULL){
                if(oldest[1]->priority!=0) {
                        tcb *tmp = oldest[1];
                        p = tmp->priority;
                        remove_tcb(&SCHEDULER->multi_level_priority_queue[p], oldest[0], oldest[1]);
                        SCHEDULER->multi_level_priority_queue[p].size--;
                        enqueue(&SCHEDULER->multi_level_priority_queue[p-1], tmp);
                        SCHEDULER->multi_level_priority_queue[p-1].size++;
                        tmp->priority++;
                }
        }
        printf("Before return\n");
        return;
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function) (void *), void *arg) {
    printf("create a thread\n");
	if (SCHEDULER_INIT == 0) { // Init scheduler if this is first time my_pthread_create is called.
		init_scheduler();
        SCHEDULER_INIT = 1;
	}
	// Create new tcb for thread.
	// Get current context.
	tcb *tcb_node = malloc(sizeof(tcb));
        T_ID++;
	tcb_node->tid = T_ID;//*thread;
        *thread = T_ID;
	if (getcontext(&(tcb_node->context)) != 0) {
		return -1; // Error getthing context
	}
	// Configure context stack.
	tcb_node->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	tcb_node->context.uc_stack.ss_flags = 0;
	tcb_node->context.uc_stack.ss_size = STACK_SIZE;
	makecontext(&(tcb_node->context), (void *) thread_function_wrapper, 3, tcb_node, function, arg);

        schedule_thread(tcb_node, 0);
    if(first_thread == 0) {
        first_thread = 1;
        execute();
    }
	if (SCHEDULER_INIT == 0) {
		// Schedule main context but don't run it.
		tcb *tcb_main_node = malloc(sizeof(tcb));
		tcb_main_node->tid = 0;
		if (getcontext(&(tcb_main_node->context)) != 0) {
			return -1; // Error getthing context.
		}
		// set context to thread that called my_pthread_create function.
		tcb_main_node->context = *(tcb_main_node->context.uc_link);
		schedule_thread(tcb_main_node, 0); // Schedule main thread.
                SCHEDULER_INIT = 1;
	}
	return 0;
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
    printf("yield\n");
	int current_priority = SCHEDULER->current_tcb->priority;
	tcb *tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
    printf("reach 1\n");
	if (tcb_node->state != TERMINATED) {
		tcb_node->state = READY;
	}
    printf("reach 2\n");
	schedule_thread(tcb_node, tcb_node->priority);
    	//check to see if we need to move on to the next queue
    	if (HAS_RUN>=SCHEDULER->multi_level_priority_queue[current_priority].size) {
        	HAS_RUN=0; //running a new queue set the counter to 0
                if (current_priority==NUMBER_LEVELS-1) {// this is the lowest priority
        		SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[0])); //run the highest priority queue
                } else {
            		SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority+1])); //run the next priority queue
                }
        } else {
                SCHEDULER->current_tcb = peek(&(SCHEDULER->multi_level_priority_queue[current_priority])); //stay in the same queue
        }
    printf("reach 3\n");
	// Swap context to new SCHEDULER->current_tcb->context, store current context to &(SCHEDULER->current_tcb->context)
	if (SCHEDULER->current_tcb->state == TERMINATED) { // Don't run context if TERMINATED
		my_pthread_yield();
		return 0;
	}
    printf("reach 4\n");
	SCHEDULER->current_tcb->state = RUNNING;
	tcb_node->last_yield_time = current_time();
    SCHEDULER->current_tcb->recent_start_time = current_time();
    HAS_RUN++;
	setcontext(&(SCHEDULER->current_tcb->context));
    
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
    int current_priority = SCHEDULER->current_tcb->priority;
	tcb *tcb_node = dequeue(&(SCHEDULER->multi_level_priority_queue[current_priority]));
	if (tcb_node->state != TERMINATED) {
        if(value_ptr != NULL)
            value_ptr = tcb_node -> return_value;
		tcb_node->state = TERMINATED;
	}
    else return;

};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
    int i;
    int flag =0;
    for(i=0; i<NUMBER_LEVELS;i++)
    {
        tcb* node = SCHEDULER->multi_level_priority_queue[i].head;
        if(node != NULL)
        {
            if(node -> tid == thread)
            {
                while(node -> state != TERMINATED)
                {

                }
                flag = 1;
            }
            else node = SCHEDULER->multi_level_priority_queue[i].head->next_tcb;
            while(node != SCHEDULER->multi_level_priority_queue[i].head)
            {
                if(node -> tid == thread)
                {
                    while(node -> state != TERMINATED)
                    {

                    }
                    flag = 1;
                    break;
                }
                else node = SCHEDULER->multi_level_priority_queue[i].head->next_tcb;
            }
            if(flag) break;
        }
        else continue;
    }
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {

    if (SCHEDULER_INIT == 0) {
        init_scheduler();
        SCHEDULER_INIT =1;
	}
	if (NUMBER_LOCKS == 0) {
		SCHEDULER->wait_queues = malloc(sizeof(queue));
		NUMBER_LOCKS++;
	} else {
		SCHEDULER->wait_queues = realloc(SCHEDULER->wait_queues, sizeof(queue) * ++NUMBER_LOCKS);
	}
	mutex = malloc(sizeof(my_pthread_mutex_t));
    mutex -> val = UNLOCKED;
	mutex->lock_owner = 0;
	mutex->lock_wait_queue = &(SCHEDULER->wait_queues[NUMBER_LOCKS - 1]); // This is this locks wait queue.

	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {

    if(SCHEDULER->current_tcb == NULL)
    {
        return -1;
    }
    int current_priority = SCHEDULER->current_tcb ->priority;
    printf("got here\n");
	tcb *tcb_node = peek(&(SCHEDULER->multi_level_priority_queue[current_priority]));
    while (__sync_lock_test_and_set(&(mutex -> val), 1))
    {

        my_pthread_mutex_t *another_lock;
        my_pthread_mutex_init(another_lock, NULL);
        my_pthread_mutex_lock(another_lock);
        //spin_lock(another_lock);
        if (mutex -> lock_owner == tcb_node->tid)
        {
            enqueue(mutex -> lock_wait_queue, tcb_node); // Put self in queue
            my_pthread_mutex_unlock(another_lock);
            while(mutex -> lock_owner != tcb_node->tid){continue;}
            //spin_unlock(another_lock);
            //Thread.sleep(); // Put self to sleep
        }
        else
        {
            my_pthread_mutex_unlock(another_lock);
        }
    }
    mutex->lock_owner = tcb_node->tid;
    // Got the lock
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
    my_pthread_mutex_t *another_lock;
    my_pthread_mutex_init(another_lock, NULL);
    my_pthread_mutex_lock(another_lock);
    tcb *next_node =dequeue(mutex -> lock_wait_queue);
    mutex -> val = UNLOCKED;
    my_pthread_mutex_unlock(another_lock);
    if (next_node != NULL)
        mutex->lock_owner = next_node->tid;
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
    tcb* node;
    while((node = dequeue (mutex -> lock_wait_queue))!= NULL){
        continue;
    }
    free(mutex -> lock_wait_queue);
    free(mutex);
	return 0;
};

void print_queue(){
    int i=0;
    int x=0;
    for(i=0;i<NUMBER_LEVELS;i++){
        int size = SCHEDULER->multi_level_priority_queue[i].size;
        tcb* p = SCHEDULER->multi_level_priority_queue[i].head;
        while(p!=NULL){
            printf("%d ",p->tid);
            p = p->next_tcb;
        }
        printf("\n");
    }
}
