#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "lib-misc.h"

#define MAX_BUF_SIZE 5

struct matrix_record {
    int matrix_size;
    int **matrix;
};

struct shared_buffer {
    struct matrix_record buffer[MAX_BUF_SIZE];
    pthread_mutex_t mutex;
    sem_t full_count;
    sem_t empty_count;
};

struct lettore_args {
    char dir[255];
    int matrix_size;
    int thread_no;
};















void * lettore(void * arg) {
    struct lettore_args * this_args = (struct lettore_args *)arg;
    int fd;
    struct stat sb;
    int *map_matrix;
    
    if ((fd = open(this_args->dir, O_RDONLY)) == -1) {
        exit_with_sys_err("open");
    }

    if (fstat(fd, &sb) == -1) {
        exit_with_sys_err("fstat");
    }

    if (!S_ISREG(sb.st_mode)) {
        exit_with_err_msg("%s non è un file regolare\n", this_args->dir);
    }
    // @todo
    /*MAPPING PART NOT WORKING CORRECTLY*/
}


int main(int argc, char *argv[]) {
    int err; /* for error handling */

    /* first I check for the correct user input */
    if (argc < 3) {
        printf("wrong usage. the correct usage is %s <M-square-size> <bin-file-1> ...  <bin-file-N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* check for the size of the semi magic squares */
    int semi_magic_square_size = atoi(argv[1]);

    if (semi_magic_square_size < 3 || semi_magic_square_size > 16) {
        printf("wrong usage. The size for the semi-magic squares has to be in the range [3,16]\n");
        exit(EXIT_FAILURE);
    }

    /* thread creation and handling */
    printf("[MAIN] I file da verificare sono %d, procedo a creare tanti thread lettori e un thread verificatore.\n", argc-2);
    pthread_t thread_lettori[argc-2];
    pthread_t thread_vericatore;

    for (int i = 0; i < argc - 2; i++) {

        struct lettore_args *lettore_data = (struct lettore_args *)malloc(sizeof(struct lettore_args));
        lettore_data->matrix_size = atoi(argv[1]);
        strcpy(lettore_data->dir, argv[i+2]);
        lettore_data->thread_no = i+1;

        if ((err = (pthread_create(&thread_lettori[i], NULL, lettore, (void *)lettore_data)) != 0)) {
            exit_with_err("pthread create", err);
        }
    }
    /*
    if ((err = (pthread_create(&thread_vericatore, NULL, NULL, NULL)) != 0)) {
        exit_with_err("pthread create", err);
    }
    */

    /* thread joining and handling */
    for (int i = 0; i < argc - 2; i++) {
        if ((err = (pthread_join(thread_lettori[i], NULL)) != 0)) {
            exit_with_err("pthread join", err);
        }
    }

    /*
    if ((err = (pthread_join(thread_vericatore, NULL)) != 0)) {
        exit_with_err("pthread join", err);
    }
    */


}