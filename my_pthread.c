// name: Patrick Currie, Ran Sa, Yaowen Zhang;
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

tQueue* multi_level_pqueue[3];
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
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

/*Create the 3 queue*/
void createQ(){
    //create and initialize 3 queues
    for(int i=0; i<3;i++){
        tQueue* t = (tQueue*)malloc(sizeof(tQueue));
        t->size=0;
        t->front=NULL;
        t->end=NULL;
        multi_level_pqueue[i]=t;
    }
}

/*Check the size of each queue*/
/*arg1: int i -> the queue you want to get the size. e.g getSize(1) will give you the size of the first priority queue*/
int getSize(int i){
    i--;
    return multi_level_pqueue[i]->size;
}
/* add a thread to the end of the queue */
/*arg1: int i -> the queue you want add the thread to. e.g  enQueue(1, t) will add thread t to the first priority queue*/
/*arg2: my_pthread_t* thread-> the thread you want to add to the queue*/
void enQueue(int i, my_pthread_t* thread){
    i--;
    //first create a node for the thread
    tNode* node = (tNode*)malloc(sizeof(tNode));
    node->value=thread;
    node->next=NULL;
    //add to the end of the queue
    if(multi_level_pqueue[i]->end==NULL){
        multi_level_pqueue[i]->front = node;
        multi_level_pqueue[i]->end = node;
    }else{
        multi_level_pqueue[i]->end->next = node;
        multi_level_pqueue[i]->end = multi_level_pqueue[i]->end->next;
    }
    multi_level_pqueue[i]->size++;
}
/* pop the first thread out of the queue */
/*arg1: int i -> the queue you want to pop the thread out. e.g  deQueue(1) will pop the first thread in the queue out*/
my_pthread_t* deQueue(int i){
    i--;
    if(multi_level_pqueue[i]->front==NULL){
        multi_level_pqueue[i]->end = NULL;
        return NULL;
    }else{
        tNode* temp = multi_level_pqueue[i]->front;
        multi_level_pqueue[i]->front = multi_level_pqueue[i]->front->next;
        my_pthread_t* t = temp->value;
        free(temp);
        multi_level_pqueue[i]->size++;
        return t;
    }
}