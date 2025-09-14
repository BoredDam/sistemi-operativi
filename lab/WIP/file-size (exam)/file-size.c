#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "lib-misc.h"
#include <linux/limits.h>

struct buffer_condiviso_t{
    char path_name[10][PATH_MAX];
    sem_t count_sem;
    sem_t reverse_count_sem;
    pthread_mutex_t mutex;
};

struct dir_thread_args{
    int thread_number;
    char path_name[PATH_MAX];
    struct buffer_condiviso_t *shared_buffer;
};

/**
 * funzione dei thread di tipo DIR
 */
void *dir_scanner(void *args) {
    struct dir_thread_args arguments = *(struct dir_thread_args *)args;
    printf("[THREAD %d] : benvenuto, procederò a scannerizzare la directory '%s'\n", arguments.thread_number, arguments.path_name);

    DIR *dp;
    

    if((dp = opendir(arguments.path_name)) == NULL) {
        exit_with_sys_err("opendir");
    };

    struct dirent *dirent_buffer;
    struct stat stat_buf;

    while ((dirent_buffer = readdir(dp)) != NULL) {
        sem_post(&arguments.shared_buffer->reverse_count_sem);
        sem_wait(&arguments.shared_buffer->count_sem);
        pthread_mutex_lock(&arguments.shared_buffer->mutex);

        if (strcmp(dirent_buffer->d_name, ".") == 0 || strcmp(dirent_buffer->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", arguments.path_name, dirent_buffer->d_name);

        if (stat(full_path, &stat_buf) == -1) {
            exit_with_sys_err(full_path);
        }

        if (!S_ISREG(stat_buf.st_mode)) {
            continue;
        }
        
        

        // inserimento nella struttura dati condivisa

        int sem_val;
        sem_getvalue(&arguments.shared_buffer->count_sem, &sem_val);
        strcpy(arguments.shared_buffer->path_name[sem_val], full_path);
        pthread_mutex_unlock(&arguments.shared_buffer->mutex);
        sem_wait(&arguments.shared_buffer->reverse_count_sem);
        sem_post(&arguments.shared_buffer->count_sem);
    }

    return (void *)NULL;
}


/**
 * funzione del thread di tipo STAT
 */
void *stat_scanner(void *args) {

    struct dir_thread_args arguments = *(struct dir_thread_args *)args;

    struct stat stat_buf;
    int sem_val;
    char file_path[PATH_MAX];
    
    //preleviamo dal buffer condiviso
    while(1) {
        printf("a");
        
        
        sem_getvalue(&arguments.shared_buffer->count_sem, &sem_val);
        strcpy(arguments.shared_buffer->path_name[sem_val], file_path);

        if (stat(file_path, &stat_buf) == -1) {
                exit_with_sys_err(file_path);
        }
        
        printf("[THREAD STAT] : %s - dimensione: %d bytes\n", file_path, stat_buf.st_size);
        
        
    }
}


int main(int argc, char *argv[]) {

    int err;

    // gestione errore d'uso del comando
    if (argc < 2) {
        printf("not enough arguments. usage: file-size <dir-1> <dir-2> ... <dir-n>\n");
        exit(EXIT_FAILURE);
    }

    // creazione thread, semafori
    int dir_count = argc - 1;
    pthread_t thread_ids[dir_count+1];
    struct dir_thread_args thread_data[dir_count];
    struct dir_thread_args stat_thread_data;
    
    // struttura dati condivisa
    struct buffer_condiviso_t shared_buffer;

    if ((err = sem_init(&shared_buffer.count_sem, PTHREAD_PROCESS_PRIVATE, 0))) {
        exit_with_err("sem_init", err);
    }

    if ((err = sem_init(&shared_buffer.reverse_count_sem, PTHREAD_PROCESS_PRIVATE, 10))) {
        exit_with_err("sem_init", err);
    }

    if ((err = pthread_mutex_init(&shared_buffer.mutex, NULL))) {
        exit_with_err("pthread_mutex_init", err);
    }
    
    for (int i = 0; i < dir_count; i++) {
        thread_data[i].thread_number = i + 1;
        strcpy(thread_data[i].path_name, argv[i + 1]);
        thread_data[i].shared_buffer = &shared_buffer;

        if ((err = pthread_create(&thread_ids[i], NULL, dir_scanner, (void *)&thread_data[i]))) {
            exit_with_err("pthread_create", err);
        }
    }

    stat_thread_data.shared_buffer = &shared_buffer;

    if ((err = pthread_create(&thread_ids[dir_count + 1], NULL, stat_scanner, (void *)&stat_thread_data))) {
        exit_with_err("pthread_create", err);
    }

    for (int i = 0; i < dir_count+1; i++) {
        if ((err = pthread_join(thread_ids[i], NULL))) {
            exit_with_err("pthread_join", err);
        }
    }

    printf("CONTROLLIAMO\n");
    return 0;
}