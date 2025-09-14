#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "lib-misc.h"

#define BUF_SIZE 10


/* 
this is our shared buffer. it's a first-in first-out queue.
mutex and semaphores are needed to achieve mutual exclusion
*/
 struct common_buffer{
    int buffer[BUF_SIZE];
    int count;
    sem_t full_count;
    sem_t empty_count;
    pthread_mutex_t mutex;
};


void thread_safe_queue_setup(struct common_buffer *buf) {

    
    for (int i = 0; i < BUF_SIZE; i++) {
        buf->buffer[i] = 0; /*initialize queue values at 0*/
    }

    buf->count = 0;
    sem_init(&buf->full_count, 0, 0);
    sem_init(&buf->empty_count, 0, BUF_SIZE);
    pthread_mutex_init(&buf->mutex, NULL);
}


void thread_safe_queue_destroy(struct common_buffer *buf) {
    sem_destroy(&buf->full_count);
    sem_destroy(&buf->empty_count);
    pthread_mutex_destroy(&buf->mutex);
}


void thread_safe_print_queue(struct common_buffer *buf) {

    pthread_mutex_lock(&buf->mutex);
    for (int i = 0; i < BUF_SIZE; i++) {
        printf("[ %d ]", buf->buffer[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&buf->mutex);

}


void thread_safe_enqueue(struct common_buffer *buf, int element) {

    sem_wait(&buf->empty_count);
    pthread_mutex_lock(&buf->mutex);
    
    buf->buffer[buf->count] = element;
    buf->count++;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->full_count);
}


int thread_safe_dequeue(struct common_buffer *buf) {

    sem_wait(&buf->full_count);
    pthread_mutex_lock(&buf->mutex);

    int out = buf->buffer[0]; /* return value */

    for (int i = 0; i < buf->count - 1; i++) {
        buf->buffer[i] = buf->buffer[i + 1]; /* moves each item forward by one slot */
    }

    buf->buffer[buf->count - 1] = 0; /* the last item in the queue becomes 0 */
    buf->count--;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->empty_count);

    return out;
}


void *consumer_func(void * args) {
    
    struct common_buffer *shared_buffer = (struct common_buffer *)args;

    for (int i = 0; i < 500; i++) {
        thread_safe_dequeue(shared_buffer);
        thread_safe_print_queue(shared_buffer);
    }
    return NULL;
}


void *producer_func(void * args) {
    
    struct common_buffer *shared_buffer = (struct common_buffer *)args;

    for (int i = 0; i < 500; i++) {
        thread_safe_enqueue(shared_buffer, rand()%10+1);
        thread_safe_print_queue(shared_buffer);
    }
    return NULL;
}


int main() {
    
    int err;
    pthread_t consumer, producer;
    struct common_buffer shared_buffer;
    
    
    thread_safe_queue_setup(&shared_buffer);
    
    if ((err = pthread_create(&consumer, NULL, consumer_func, (void *)&shared_buffer)) != 0) {
        exit_with_err("pthread_create", err);
    }

    if ((err = pthread_create(&producer, NULL, producer_func, (void *)&shared_buffer)) != 0) {
        exit_with_err("pthread_create", err);
    }

    if (( err = pthread_join(consumer, NULL)) != 0) {
        exit_with_err("pthread_join", err);
    }

    if (( err = pthread_join(producer, NULL)) != 0) {
        exit_with_err("pthread_join", err);
    }

    thread_safe_queue_destroy(&shared_buffer);
}