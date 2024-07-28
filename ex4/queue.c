
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
    int total_visited;
    int size;
    mtx_t mutex;
    cnd_t not_empty;

} Queue;

typedef struct Thread_queue {
    ThreadNode* head;
    ThreadNode* tail;
    int waiting_threads;
    cnd_t wake_to_dequeue;
} Thread_queue;

Queue* queue;
Thread_queue* threads;

/*void printQueue(void){
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
}*/

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
    threads->waiting_threads++;
}

void removeThreadTail(void){
    if (threads->waiting_threads == 1){
        free(threads->tail);
        threads->tail = NULL;
        threads->head = NULL;
        threads->waiting_threads--;
    }
    else if (threads->waiting_threads > 1){
        ThreadNode* temp = threads->tail->prev;
        free(threads->tail);
        threads->tail = temp;
        threads->tail->next = NULL;
        threads->waiting_threads--;
    }
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
    queue->size++;
}

void removeQueueTail(void){
    if (queue->size == 1){
        free(queue->tail);
        queue->tail = NULL;
        queue->head = NULL;
        queue->size--;
    }
    else if (queue->size > 1){
        Node* temp = queue->tail->prev;
        free(queue->tail);
        queue->tail = temp;
        queue->tail->next = NULL;
        queue->size--;
    }
}

void initQueue(void) {
    // Initialize the queue
    queue = malloc(sizeof(Queue));
    threads = malloc(sizeof(Thread_queue));
    queue->head = NULL;
    queue->tail = NULL;
    threads->head = NULL;
    threads->tail = NULL;
    threads->waiting_threads = 0;
    queue->size = 0;
    queue->total_visited = 0;
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
    mtx_lock(&queue->mutex);
    Node* new_node = malloc(sizeof(Node));
    new_node->data = item;
    addNode(new_node);
    queue->total_visited++;
    // wake up the thread that is in tail and let him dequeue the item
    if (threads->waiting_threads > 0 && queue->size > 0){
        cnd_signal(&threads->tail->cond);
    }
    mtx_unlock(&queue->mutex);
    return;
}

void* dequeue(void){
    mtx_lock(&queue->mutex);
    // get the last thread that is waiting
    if (threads->waiting_threads > 0 || queue->size == 0){        
        // put the thread to sleep until there is an item in the queue
        // add the thread to the list of threads that are waiting
        ThreadNode* new_thread = malloc(sizeof(ThreadNode));
        new_thread->thread = thrd_current();
        cnd_init(&new_thread->cond);
        addThread(new_thread);
        cnd_wait(&new_thread->cond, &queue->mutex);
    }
    
    // remove the thread from the tail of threads that are waiting and dequeue the item
    void* item = queue->tail->data;
    removeThreadTail();
    removeQueueTail();
    
    if (threads->waiting_threads > 0 && queue->size > 0){
        cnd_signal(&threads->tail->cond);
    }
    
    mtx_unlock(&queue->mutex);
    return item;
}

bool tryDequeue(void** item) {

    mtx_lock(&queue->mutex);
    if(queue->size == 0 ){        
        mtx_unlock(&queue->mutex);
        return false;
    } else{
        *item = queue->tail->data;
        // add the thread to the list of threads that are waiting
        ThreadNode* new_thread = malloc(sizeof(ThreadNode));
        new_thread->thread = thrd_current();
        new_thread->next = NULL;
        cnd_init(&new_thread->cond);
        addThread(new_thread);
        // wake up the thread that is in tail and let him dequeue the item
        if(threads->waiting_threads > 0 && queue->size > 0){
            cnd_signal(&threads->tail->cond);
        }   
        // remove the thread from the tail of threads that are waiting
        removeThreadTail();
        // dequeue the item
        removeQueueTail();
        
        mtx_unlock(&queue->mutex);
        return true;
    }
}

size_t size(void) {
    return queue->size;
}

size_t waiting(void) {
    return threads->waiting_threads;
}

size_t visited(void) {
    return queue->total_visited;
}
