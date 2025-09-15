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

struct square_record {
    int matrix[16][16];
    int thread_number;
    int square_number;
};

struct shared_buffer {
    struct square_record buffer[MAX_BUF_SIZE];
    int count;
    pthread_mutex_t mutex;
    sem_t full_count;
    sem_t empty_count;
    pthread_mutex_t print_mutex;
};

struct thread_args {
    char dir[255];
    int matrix_size;
    int thread_no;
    struct shared_buffer *queue;
};


void queue_setup(struct shared_buffer * buf) {

    if (sem_init(&buf->empty_count, 0, MAX_BUF_SIZE) != 0) {
        exit_with_err_msg("sem_init");
    }

    if (sem_init(&buf->full_count, 0, 0) != 0) {
        exit_with_err_msg("sem_init");
    }
    
    if (pthread_mutex_init(&buf->mutex, NULL) != 0) {
        exit_with_err_msg("mutex_init");
    }

    if (pthread_mutex_init(&buf->print_mutex, NULL) != 0) {
        exit_with_err_msg("mutex_init");
    }

    buf->count = 0;

}

void safe_enqueue(struct shared_buffer * buf, struct square_record rec) {
    
    sem_wait(&buf->empty_count);
    pthread_mutex_lock(&buf->mutex);

    buf->buffer[buf->count] = rec;
    buf->count++;
    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->full_count);

}

struct square_record safe_dequeue(struct shared_buffer * buf) {
    
    sem_wait(&buf->full_count);
    pthread_mutex_lock(&buf->mutex);

    struct square_record head = buf->buffer[0];
    for (int i = 0; i < buf->count-1; i++) {
        buf->buffer[i] = buf->buffer[i+1];
    }

    buf->count--;
    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->empty_count);

    return head;
}






void * lettore(void * arg) {

    struct thread_args * this_args = (struct thread_args *)arg;
    int semi_magic_square_size = this_args->matrix_size;
    int fd;
    struct stat sb;
    unsigned char *data;
    struct square_record read_record;
    
    if ((fd = open(this_args->dir, O_RDONLY)) == -1) {
        exit_with_sys_err("open");
    }

    /* fstat is needed for both file regularity and size checking*/
    if (fstat(fd, &sb) == -1) {
        exit_with_sys_err("fstat");
    }

    if (!S_ISREG(sb.st_mode)) {
        exit_with_err_msg("%s non è un file regolare\n", this_args->dir);
    }


    int semi_magic_squares_size_in_bytes = (sizeof(int)*semi_magic_square_size*semi_magic_square_size);

    int semi_magic_squares_number = sb.st_size/semi_magic_squares_size_in_bytes;

    data = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    for (int n = 0; n < semi_magic_squares_number; n++) {
        
        pthread_mutex_lock(&this_args->queue->print_mutex);
        printf("[THREAD READER %d] leggo il quadrato numero %d:\n", this_args->thread_no, n);
        for (int i = 0; i < semi_magic_square_size; i++) {

            for (int j = 0; j < semi_magic_square_size; j++) {
                read_record.matrix[i][j] = data[n*semi_magic_squares_size_in_bytes+i*semi_magic_square_size+j];
                printf("%d ", data[n*semi_magic_squares_size_in_bytes+i*semi_magic_square_size+j]);
            }
            printf("\n");
        }

        read_record.thread_number = this_args->thread_no;
        read_record.square_number = n;
        safe_enqueue(this_args->queue, read_record);
        printf("\n");
        pthread_mutex_unlock(&this_args->queue->print_mutex);
    }

    return NULL;
}

void * verificatore(void * arg) {
    struct thread_args * this_args = (struct thread_args *)arg;
    int semi_magic_square_size = this_args->matrix_size;
    struct square_record read_record;
    int row_sum;
    int last_sum = 0;
    int is_magic = 1;

    while (1) {
        is_magic = 1;
        read_record = safe_dequeue(this_args->queue);
        printf("[VERIFIER]: verifico...\n");
        for (int i = 0; i < semi_magic_square_size; i++) {
            row_sum = 0;
            for (int j = 0; j < semi_magic_square_size; j++) {
                row_sum = row_sum + read_record.matrix[i][j];
            }
            if (i == 0) {
                last_sum = row_sum;
            }
            if (row_sum != last_sum) {
                is_magic = 0;
            }
            
        }

        for (int j = 0; j < semi_magic_square_size; j++) {
            row_sum = 0;
            for (int i = 0; i < semi_magic_square_size; i++) {
                row_sum = row_sum + read_record.matrix[i][j];
            }
            if (row_sum != last_sum) {
                is_magic = 0;
            }
            
        }

        if (is_magic == 1) {
            printf("[VERIFIER]: il quadrato %d del reader %d è magico! Somma magica: %d\n", read_record.square_number, read_record.thread_number, last_sum);
        }
        
        last_sum = 0;
    }
}


int main(int argc, char *argv[]) {
    int err; /* for error handling */
    struct shared_buffer buf;

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

    queue_setup(&buf);

    /* thread creation and handling */
    printf("[MAIN] I file da verificare sono %d, procedo a creare tanti thread lettori e un thread verificatore.\n", argc-2);
    pthread_t thread_lettori[argc-2];
    pthread_t thread_vericatore;

    for (int i = 0; i < argc - 2; i++) {

        struct thread_args *lettore_data = (struct thread_args *)malloc(sizeof(struct thread_args));
        lettore_data->matrix_size = atoi(argv[1]);
        strcpy(lettore_data->dir, argv[i+2]);
        lettore_data->thread_no = i+1;
        lettore_data->queue = &buf;

        if ((err = (pthread_create(&thread_lettori[i], NULL, lettore, (void *)lettore_data)) != 0)) {
            exit_with_err("pthread create", err);
        }
    }
    
    /* verifier datas are mainly placeholder or null datas. just wanted to reuse the same struct used for the reader threads */
    struct thread_args *verificatore_data = (struct thread_args *)malloc(sizeof(struct thread_args));
    verificatore_data->matrix_size = atoi(argv[1]);
    strcpy(verificatore_data->dir, " ");
    verificatore_data->thread_no = 0;
    verificatore_data->queue = &buf;

    if ((err = (pthread_create(&thread_vericatore, NULL, verificatore, (void *)verificatore_data)) != 0)) {
        exit_with_err("pthread create", err);
    }
    

    /* thread joining and handling */
    for (int i = 0; i < argc - 2; i++) {
        if ((err = (pthread_join(thread_lettori[i], NULL)) != 0)) {
            exit_with_err("pthread join", err);
        }
    }

    
    if ((err = (pthread_join(thread_vericatore, NULL)) != 0)) {
        exit_with_err("pthread join", err);
    }
    
    printf("[MAIN]: fine!\n");

}