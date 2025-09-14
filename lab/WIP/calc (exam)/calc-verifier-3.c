#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_REQUESTS 5
#define CHAR_BUFFER_MAX 100


struct request{
    int thread_id;
    long long add1;
    long long add2;
    char op_type;
};

struct request_buffer{
    struct request req_buffer[MAX_REQUESTS];
    long long * response_buffer;
    int closed_threads;
    int good_verifies;
};

struct thread_args{
    struct request_buffer * shared_buffer_ref;
    FILE * fd;
};

void * file_thread(void * args){

    struct thread_args input_args = *(struct thread_args *)args;
    char buf[CHAR_BUFFER_MAX];
    fgets(buf, CHAR_BUFFER_MAX, input_args.fd);
    int last_calc = atoi(buf);
    char op_buffer;
    printf("%d", last_calc);
    
    struct request req_to_evaluate;
    while((op_buffer = fgetc(input_args.fd)) == '+' || op_buffer == '-' || op_buffer == 'x' ) {
        printf("%c", op_buffer);
        req_to_evaluate.op_type = op_buffer;
        fgets(buf, CHAR_BUFFER_MAX, input_args.fd);
        req_to_evaluate.add2 = atoi(buf);
        printf("%ld", req_to_evaluate.add2);
    }
    //input_args.shared_buffer_ref->req_buffer[1];



    fclose(input_args.fd);
}

void * calc_thread(void * args){
    
}
















int main(int argc, char *argv[]) {
    
    if(argc<=1) {
        printf("wrong amount of arguments.\n");
        return EXIT_FAILURE;
    }

    struct request_buffer shared_buffer;
    shared_buffer.response_buffer = (long long *) malloc(sizeof(long long)*(argc-1));
    pthread_t threads[argc];
    struct thread_args thread_arg_array[argc];

    for(int i = 1; i <= argc; i++) {
        thread_arg_array[i].fd = fopen(argv[i], "r");
        pthread_create(&threads[i], NULL, file_thread, (void *)&thread_arg_array[i]);
    }

}