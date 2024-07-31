
#include "queue.h"
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>

// ================================== data structures ==================================//
// Node structure
typedef struct Node {
    void* data;
    struct Node* next;
    struct Node* prev;
} Node;
// ThreadNode structure
typedef struct ThreadNode {
    cnd_t cond;
    struct ThreadNode* next;
    struct ThreadNode* prev;
} ThreadNode;
// Queue structure
typedef struct Queue {
    Node* head;
    Node* tail;
    int total_visited;
    int size;
    mtx_t mutex;

} Queue;
// Thread_queue structure
typedef struct Thread_queue {
    ThreadNode* head;
    ThreadNode* tail;
    int waiting_threads;
} Thread_queue;

// ================================== global variables ==================================//
Queue* queue;
Thread_queue* threads;

// ================================= helper functions =================================

// a helper function to add a thread to the thread queue
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

// a helper function to remove the tail of the thread queue
void removeThreadTail(void){
    if (threads->waiting_threads == 1){
        cnd_destroy(&threads->tail->cond);
        free(threads->tail);
        threads->tail = NULL;
        threads->head = NULL;
        threads->waiting_threads--;
    }
    else if (threads->waiting_threads > 1){
        ThreadNode* temp = threads->tail->prev;
        cnd_destroy(&threads->tail->cond);
        free(threads->tail);
        threads->tail = temp;
        threads->tail->next = NULL;
        threads->waiting_threads--;
    }
}

// a helper function to add a node to the queue
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

// a helper function to remove the tail of the queue
void removeQueueTail(void){
    if (queue->size == 1){
        free(queue->tail);
        queue->tail = NULL;
        queue->head = NULL;
        queue->size--;
        queue->total_visited++;
    }
    else if (queue->size > 1){
        Node* temp = queue->tail->prev;
        free(queue->tail);
        queue->tail = temp;
        queue->tail->next = NULL;
        queue->size--;
        queue->total_visited++;
    }
    else{
        return;
    }
}

// a helper function to remove a node from the queue
void removeNode(Node* node){
    if (node->prev == NULL){
        queue->head = node->next;
        queue->head->prev = NULL;
    }
    else if (node->next == NULL){
        queue->tail = node->prev;
        queue->tail->next = NULL;
    }
    else{
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    free(node);
    queue->size--;
    queue->total_visited++;
}
// ================================== initialization ==================================
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
}

// ================================== destruction ==================================
void destroyQueue(void){
    // Destroy the queue
    if (queue == NULL){
        return;
    }
    mtx_lock(&queue->mutex);
    while (queue->size > 0){
        removeQueueTail();
    }
    while (threads->waiting_threads > 0){
        removeThreadTail();
    }
    mtx_unlock(&queue->mutex);
    mtx_destroy(&queue->mutex);
   
}
// ================================== queue operations ==================================
void enqueue(void* item){
    mtx_lock(&queue->mutex);
    Node* new_node = malloc(sizeof(Node));
    new_node->data = item;
    addNode(new_node);
    // wake up the thread that is in tail and let him dequeue the item
    if (threads->waiting_threads > 0 && queue->size > 0){
        cnd_signal(&threads->tail->cond);
    }
    mtx_unlock(&queue->mutex);
    return;
}

void* dequeue(void) {
    mtx_lock(&queue->mutex);
    void* item;
    if(threads -> waiting_threads > 0 || queue -> size == 0){
        // create a new thread and wait for it to dequeue the item
        ThreadNode* new_thread = malloc(sizeof(ThreadNode));
        cnd_init(&new_thread->cond);
        addThread(new_thread);
        cnd_wait(&new_thread->cond, &queue->mutex);   
    }
    item = queue->tail->data;
    removeQueueTail();
    removeThreadTail();
    // wake up the thread that is in tail and let him dequeue the item
    if (threads->waiting_threads > 0 && queue->size > 0){
        cnd_signal(&threads->tail->cond);
    }
    mtx_unlock(&queue->mutex);
    return item;
} 

bool tryDequeue(void** item) {
    mtx_lock(&queue->mutex);
    // if there are more waiting threads than the size of the queue, return false
    if(threads->waiting_threads >= queue->size){  
        mtx_unlock(&queue->mutex);
        return false;
    }
    else if (threads->waiting_threads == 0){
        *item = queue -> tail -> data;
        removeQueueTail();
    }
    // if there are waiting threads, bypass the waiting threads and dequeue the item
    else{
        Node* temp = queue->tail;
        for (int i = 0; i < threads->waiting_threads; i++){
        temp = temp->prev;
        }   
        *item = temp->data;
        removeNode(temp);
    }
    
    mtx_unlock(&queue->mutex);
    return true;
    
}
// ================================== queue information ==================================//
size_t size(void) {
    mtx_lock(&queue->mutex);
    size_t size = queue->size;
    mtx_unlock(&queue->mutex);
    return size;
}

size_t waiting(void) {
    return threads->waiting_threads;
}

size_t visited(void) {
    mtx_lock(&queue->mutex);
    size_t visited = queue->total_visited;
    mtx_unlock(&queue->mutex);
    return visited;
}
