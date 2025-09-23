#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "lib-misc.h"

#define BUF_SIZE 10

struct dir_thread_arguments{
    char path_name[255];
    struct queue_buffer *buf;
};

struct search_thread_arguments{
    char word[255];
    char directory[255];
    struct queue_buffer *buf;
};

struct file_record{
    char path_name[255];
    int occurrences;
};

struct queue_buffer{
    struct file_record buf[BUF_SIZE];
    int count;
    sem_t full;
    sem_t empty;
    pthread_mutex_t mutex;
};

void buffer_init(struct queue_buffer *queue) {
    int err;
    
    if ((err = sem_init(&queue->full, 0, 0)) != 0) {
        exit_with_err("sem_init", err);
    }
    
    if (err = sem_init(&queue->empty, 0, BUF_SIZE) != 0) {
        exit_with_err("sem_init", err);
    }
    
    if (err = pthread_mutex_init(&queue->mutex, NULL) != 0) {
        exit_with_err("mutex_init", err);
    }

    queue->count = 0;
    printf("queue setupped succesfully\n");
}

void safe_enqueue(struct queue_buffer *queue, struct file_record record) {

    sem_wait(&queue->empty);
    pthread_mutex_lock(&queue->mutex);

    queue->buf[queue->count] = record;
    queue->count++;

    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->full);
}

struct file_record safe_dequeue(struct queue_buffer *queue) {

    sem_wait(&queue->full);
    pthread_mutex_lock(&queue->mutex);
    
    struct file_record to_return = queue->buf[0];

    for (int i = 0; i < queue->count - 1; i++) {
        queue->buf[i] = queue->buf[i + 1];
    }
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->empty);
}

void * dir_routine(void * thread_args) {
    
    struct dir_thread_arguments arg = *(struct dir_thread_arguments*)thread_args;
    
    DIR * dir = opendir(arg.path_name);

    if (dir == NULL) {
        exit_with_err_msg("directory %s inesistente\n", arg.path_name);
    }
    struct dirent *directory_record;
    struct stat sb;
    
    strcat(arg.path_name, "/");
    char dir_prefix[255];
    strcpy(dir_prefix, arg.path_name);
    
    directory_record = readdir(dir);
    stat(directory_record->d_name, &sb);

    while (directory_record != NULL) {    
        struct file_record * record_to_enqueue = (struct file_record*) malloc(sizeof(struct file_record));
        if (S_ISREG(sb.st_mode)) {
            strcat(arg.path_name, directory_record->d_name);
            strcpy(record_to_enqueue->path_name,arg.path_name);
            printf("%s\n", record_to_enqueue->path_name);
            safe_enqueue(arg.buf, *record_to_enqueue);
            strcpy(arg.path_name, dir_prefix);
        }

        directory_record = readdir(dir);
        stat(directory_record->d_name, &sb);
        
    } 
    return NULL;

}

void * search_routine(void * thread_args) {
    struct dir_thread_arguments arg = *(struct dir_thread_arguments*)thread_args;
   
    while(1) {
        safe_dequeue(&arg.buf);
    }
}





int main(int argc, char *argv[]) {

    /*for error handling*/
    int err;

    /*input handling*/
    if (argc < 2) {
        exit_with_err_msg("correct usage is: %s <word> <dir-1> <dir-2> ... <dir-n>", argv[0]);
    }

    char word[255];
    strcpy(word, argv[1]);

    /*shared buffers init*/
    struct queue_buffer dir_to_search, search_to_main;

    buffer_init(&dir_to_search);
    buffer_init(&search_to_main);

    /*thread creation*/
    int dir_thread_number = argc - 2;
    pthread_t dir_threads[dir_thread_number];
    
    for (int i = 0; i < dir_thread_number; i++) {

        struct dir_thread_arguments *dir_thread_data = (struct dir_thread_arguments *) 
                                                        malloc(sizeof(struct dir_thread_arguments));
        strcpy(dir_thread_data->path_name, argv[i+2]);
        dir_thread_data->buf = &dir_to_search;
        if ((err = pthread_create(&dir_threads[i], NULL, dir_routine, dir_thread_data)) != 0) {
            exit_with_err("pthread_create", err);
        }

        if ((err = pthread_create(&dir_threads[i], NULL, search_routine, dir_thread_data)) != 0) {
            exit_with_err("pthread_create", err);
        }
    }

    for (int i = 0; i < dir_thread_number; i++) {
        if ((err = pthread_join(dir_threads[i], /*to do : return handling*/NULL)) != 0) {
            exit_with_err("pthread_join", err);
        }
    }

}