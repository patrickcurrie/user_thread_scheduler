// name: Patrick Currie
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

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

tQueue* createQ(int capacity, int p){
    //allocate memory for the queue
    tQueue* q = (tQueue*)malloc(sizeof(tQueue));
    q->p = p;
    q->capacity=capacity;
    q->size=0;
    q->front=-1;
    q->end=-1;
    q->tArray = (my_pthread_t*) malloc(capacity* sizeof(my_pthread_t));
    return q;
}
/* check if a queue is almost full */
/* return 1 when it is almost full; 0 otherwise */
int isFull(tQueue* tQ){
    if(tQ->size>=(tQ->capacity)*0.8){
        return 1;
    }
    return 0;
}
/* expand a queue when it is almost full */
void expandQ(int newCapacity, tQueue* tQ){
    my_pthread_t* newArray = (my_pthread_t*) malloc(newCapacity* sizeof(my_pthread_t));
    for(int i=0; i<tQ->size;i++){
        newArray[i] = deQueue(tQ);
    }
    my_pthread_t* oldArray = tQ->tArray;
    tQ->tArray=NULL;
    free(oldArray);
    tQ->tArray = newArray;
    tQ->front=0;
    tQ->end = tQ->size-1;
    tQ->capacity = newCapacity;
}
/* add a thread to the end of the queue */
void enQueue(tQueue* tQ, my_pthread_t* thread){
    //first check if there is enough room for the new thread, if not expand the queue
    if(isFull(tQ)==1){
        expandQ(tQ->capacity*2,tQ);
    }
    //then add to the end of the queue
    if(tQ->end==tQ->capacity-1){
        tQ->end = 0;
    }else{
        tQ->end=tQ->end+1;
    }
    tQ->tArray[tQ->end] = thread;
    tQ->size++;
    if(tQ->front==-1) {
        tQ->front = 0;
    }
}
/* pop the first thread out of the queue */
my_pthread_t* deQueue(tQueue* tQ){
    //first check if the queue is empty
    if(tQ->size==0){
        return NULL;
    }
    my_pthread_t* r = NULL;
    r = tQ->tArray[tQ->end];
    tQ->tArray[tQ->end]=NULL;
    if(tQ->end==0 && tQ->size>1){
        tQ->end=tQ->capacity-1;
    }else{
        tQ->end--;
    }
    tQ->size--;
    return r;
}