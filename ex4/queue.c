
#include <queue.h>
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
    atomic_size_t waiting_threads;
    atomic_size_t total_visited;
    atomic_size_t size;
    mtx_t mutex;
    cnd_t not_empty;

} Queue;

typedef struct Thread_queue {
    ThreadNode* head;
    ThreadNode* tail;
    //mtx_t mutex;
} Thread_queue;

Queue* queue;
Thread_queue* threads;

void initQueue(void) {
    // Initialize the queue
    queue = malloc(sizeof(Queue));
    threads = malloc(sizeof(Thread_queue));
    queue->head = NULL;
    queue->tail = NULL;
    atomic_init(&queue->waiting_threads, 0);
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
    mtx_lock(&queue->mutex);
    Node* new_node = malloc(sizeof(Node));
    new_node->data = item;

    if(queue->tail== NULL){
        queue->tail = new_node;
    } else {
        queue->head->prev = new_node;
        new_node->next = queue->head; 
        queue->head = new_node;
    }
    atomic_fetch_add(&queue->size, 1);
    atomic_fetch_add(&queue->total_visited, 1);
    if(atomic_load(&queue->waiting_threads) > 0){
        cnd_signal(&queue->not_empty);
    }
    mtx_unlock(&queue->mutex);
    
}

void* dequeue(void){
    // get the last thread that is waiting
    mtx_lock(&queue->mutex);
    atomic_fetch_add(&queue->waiting_threads, 1);
    while(atomic_load(&queue->size) == 0){
        // put the thread to sleep until there is an item in the queue
        // add the thread to the list of threads that are waiting
        ThreadNode* new_thread = malloc(sizeof(Thread_node));
        new_thread->thread = thrd_current();
        new_thread->next = NULL;
        new_thread->cond = cnd_init();

        if(threads->head == NULL){
            threads->head = new_thread;
            threads->tail = new_thread;
        } else {
            threads->tail->next = new_thread;
            threads->tail = new_thread;
        }

        mtx_unlock(&queue->mutex);
        // put the thread to sleep
        cnd_wait(&queue->not_empty, &queue->mutex);
    }
    atomic_fetch_sub(&queue->size, 1);
    atomic_fetch_sub(&queue->waiting_threads, 1);
    // wake up the thread that is in tail and let him dequeue the item
    mtx_unlock(&queue->mutex);
    cnd_signal(&threads->tail->cond);
    mtx_lock(&queue->mutex);
    // remove the thread from the tail of threads that are waiting
    free(threads->tail);
    Thread_queue->tail = Thread_queue->tail->prev;
    // dequeue the item
    Node* temp = queue->tail;
    void* item = temp->data;
    queue->tail = queue->tail->prev;
    free(temp);
    mtx_unlock(&Thread_queue->mutex);
    return item;
}

bool tryDequeue(void** item) {

    mtx_lock(&queue->mutex);
    atomic_fetch_add(&queue->waiting_threads, 1);
    if(atomic_load(&queue->size) == 0){
        return false;
    }
    else{
        atomic_fetch_sub(&queue->size, 1);
        atomic_fetch_sub(&queue->waiting_threads, 1);
        Node* temp = queue->tail;
        *item = temp->data;
        queue->tail = queue->tail->prev;
        free(temp); 
    }
    mtx_unlock(&queue->mutex);
    return true;
}

size_t size(void) {
    return atomic_load(&queue->size);
}

size_t waiting(void) {
    return atomic_load(&queue->waiting_threads);
}

size_t visited(void) {
    return atomic_load(&queue->total_visited);
}
