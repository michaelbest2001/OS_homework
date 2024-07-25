
#include "queue.h"
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>

typedef struct Node {
    void* data;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct ThreadNode {
    thrd_t thread;
    cnd_t cond;
    struct ThreadNode* next;
    struct ThreadNode* prev;
} ThreadNode;

typedef struct Queue {
    Node* head;
    Node* tail;
    atomic_size_t total_visited;
    atomic_size_t size;
    mtx_t mutex;
    cnd_t not_empty;

} Queue;

typedef struct Thread_queue {
    ThreadNode* head;
    ThreadNode* tail;
    atomic_size_t waiting_threads;
    //mtx_t mutex;
} Thread_queue;

Queue* queue;
Thread_queue* threads;

void printQueue(void){
    Node* temp = queue->head;
    printf("##### printing the queue #####\n");
    while(temp != NULL){
        printf("the item is %d\n", *(int*)temp->data);
        temp = temp->next;
    }
    printf("the head of the queue is %d\n", *(int*)queue->head->data);
    printf("the tail of the queue is %d\n", *(int*)queue->tail->data);
    printf("##### end of the queue #####\n");
}

void printThreads(void){
    ThreadNode* temp = threads->head;
    printf("##### printing the threads #####\n");
    while(temp != NULL){
        printf("thread : %ld\n", temp->thread);
        temp = temp->next;
    }
    if (threads->head == NULL){
        printf("the head of the threads is NULL\n");
    }
    else{
        printf("the head of the threads is %ld\n", threads->head->thread);
    }
    if (threads->tail == NULL){
        printf("the tail of the threads is NULL\n");
    }
    else{
        printf("the tail of the threads is %ld\n", threads->tail->thread);
    }

    printf("##### end of the threads #####\n");
}

void addThread(ThreadNode* thread){
    if (threads->head == NULL){
        threads->head = thread;
        threads->tail = thread;
    }
    else{
        thread->next = threads->head;
        threads->head->prev = thread;
        threads->head = thread;
    }
    atomic_fetch_add(&threads->waiting_threads, 1);
}

void removeThreadTail(void){
    if (threads->tail == NULL){
        return;
    }
    else if (threads->tail == threads->head){
        free(threads->tail);
        threads->tail = NULL;
        threads->head = NULL;
    }
    else{
        ThreadNode* temp = threads->tail;
        threads->tail = threads->tail->prev;
        threads->tail->next = NULL;
        free(temp);
    }
    atomic_fetch_sub(&threads->waiting_threads, 1);
}

void wakeUpThread(ThreadNode* thread){
    cnd_signal(&thread->cond);
}

void addNode(Node* node){
    if (queue->head == NULL){
        queue->head = node;
        queue->tail = node;
    }
    else{
        node->next = queue->head;
        queue->head->prev = node;
        queue->head = node;
    }
    atomic_fetch_add(&queue->size, 1);
}

void removeQueueTail(void){
    if (queue->tail == NULL){ 
        return;
    }
    else if (queue->tail == queue->head){
        free(queue->tail);
        queue->tail = NULL;
        queue->head = NULL;
    }
    else{
        Node* temp = queue->tail;
        queue->tail = queue->tail->prev;
        queue->tail->next = NULL;
        free(temp);
    }
    atomic_fetch_sub(&queue->size, 1);
}

void initQueue(void) {
    // Initialize the queue
    queue = malloc(sizeof(Queue));
    threads = malloc(sizeof(Thread_queue));
    queue->head = NULL;
    queue->tail = NULL;
    threads->head = NULL;
    threads->tail = NULL;
    atomic_init(&threads->waiting_threads, 0);
    atomic_init(&queue->total_visited, 0);
    atomic_init(&queue->size, 0);
    mtx_init(&queue->mutex, mtx_plain);
    cnd_init(&queue->not_empty);
}

void destroyQueue(void){
    // Destroy the queue
    mtx_destroy(&queue->mutex);
    cnd_destroy(&queue->not_empty);
    free(threads);
    free(queue);

}

void enqueue(void* item){
    //printThreads();
    mtx_lock(&queue->mutex);
    Node* new_node = malloc(sizeof(Node));
    new_node->data = item;
    addNode(new_node);
    atomic_fetch_add(&queue->total_visited, 1);
    
    if(atomic_load(&threads->waiting_threads) > 0){
        cnd_signal(&threads->tail->cond);
    }
    mtx_unlock(&queue->mutex);
    return;
}

void* dequeue(void){
    mtx_lock(&queue->mutex);
    //printf("thread num %ld in dequeu\n", thrd_current());
    // get the last thread that is waiting
    if(atomic_load(&threads->waiting_threads) >= atomic_load(&queue->size)){
        // put the thread to sleep until there is an item in the queue
        // add the thread to the list of threads that are waiting
        ThreadNode* new_thread = malloc(sizeof(ThreadNode));
        new_thread->thread = thrd_current();
        new_thread->next = NULL;
        cnd_init(&new_thread->cond);
        addThread(new_thread);
        cnd_wait(&new_thread->cond, &queue->mutex);
    }
    // remove the thread from the tail of threads that are waiting
    removeThreadTail();
    // dequeue the item
    void* item = queue->tail->data;
    removeQueueTail();
    mtx_unlock(&queue->mutex);
    return item;
    //printf("thread %ld dequeued the item\n", thrd_current());
}

bool tryDequeue(void** item) {

    mtx_lock(&queue->mutex);
    //printf("thread num %ld in try dequeu\n", thrd_current());
    if(queue->head == NULL){
        mtx_unlock(&queue->mutex);
        return false;
    }
    else{
        *item = queue->tail->data;
        // add the thread to the list of threads that are waiting
        ThreadNode* new_thread = malloc(sizeof(ThreadNode));
        new_thread->thread = thrd_current();
        new_thread->next = NULL;
        cnd_init(&new_thread->cond);
        addThread(new_thread);
        // wake up the thread that is in tail and let him dequeue the item
        if(atomic_load(&threads->waiting_threads) > 0){
            //printThreads();
            cnd_signal(&threads->tail->cond);
            
            // remove the thread from the tail of threads that are waiting
            removeThreadTail();
            // dequeue the item
            removeQueueTail();
        }
        else{
            removeQueueTail();
            
        }
        mtx_unlock(&queue->mutex);
        return true;
    }
}

size_t size(void) {
    return atomic_load(&queue->size);
}

size_t waiting(void) {
    return atomic_load(&threads->waiting_threads);
}

size_t visited(void) {
    return atomic_load(&queue->total_visited);
}
