#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


struct thread_args{
    char thread_name[30];
    pthread_mutex_t *mutex;
};


void *ping (void *args){

    struct thread_args arguments = *(struct thread_args *)args;

    for(int i = 0; i < 10; i++) {
        pthread_mutex_lock(arguments.mutex);
        printf("%s : ping!\n", arguments.thread_name);
        pthread_mutex_unlock(arguments.mutex);
        sleep(1);
    }

    return NULL;
}

void *pong (void *args){
    
    struct thread_args arguments = *(struct thread_args *)args;

    for(int i = 0; i < 10; i++) {
        pthread_mutex_lock(arguments.mutex);
        printf("%s : pong!\n", arguments.thread_name);
        pthread_mutex_unlock(arguments.mutex);
        sleep(1);
    }

    return NULL;
}


int main() {

    pthread_mutex_t mutex_ball;
    pthread_mutex_init(&mutex_ball, NULL);

    pthread_t thread_ping, thread_pong;
    struct thread_args ping_args;
    struct thread_args pong_args;
    strcpy(ping_args.thread_name, "pigger");
    strcpy(pong_args.thread_name, "pogger");
    ping_args.mutex = &mutex_ball;
    pong_args.mutex = &mutex_ball;



    pthread_create(&thread_ping, NULL, ping, (void *)&ping_args);
    pthread_create(&thread_pong, NULL, pong, (void *)&pong_args);

    pthread_join(thread_ping, NULL);
    pthread_join(thread_pong, NULL);

    pthread_mutex_destroy(&mutex_ball);
    printf("mutex destroyed\n");
    printf("bye!\n");

}