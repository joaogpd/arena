#include "../arena.h"
#include <bits/pthreadtypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define OFFSET 200

pthread_t thread_id_1 = 0;
pthread_t thread_id_2 = 0;

arena_t arena_id = -1;

void* thread_routine_1(void* arg) {
    while (1) {
        sleep(random() % 5);

        int size = ((random() % 3) + 1) * 100;

        uint8_t* memaddr = (uint8_t*)arena_request_memory(arena_id, size);

        printf("Thread1 got memaddr %p of size %d\n", memaddr, size);

        sleep(random() % 5);

        if (memaddr != NULL) {
            for (int i = 0; i < 100; i++) {
                memaddr[i] = i;
            }

            arena_free_memory(arena_id, memaddr);
            printf("Thread1 freed address\n");
        }
    }

    return NULL;
}

void* thread_routine_2(void* arg) {
    while (1) {
        sleep(random() % 5);

        int size = ((random() % 3) + 1) * 200;

        uint8_t* memaddr = (uint8_t*)arena_request_memory(arena_id, size);

        printf("Thread2 got memaddr %p of size %d\n", memaddr, size);

        sleep(random() % 5);

        if (memaddr != NULL) {
            for (int i = 0; i < 100; i++) {
                memaddr[i] = i;
            }

            arena_free_memory(arena_id, memaddr);
            printf("Thread2 freed address\n");
        }
    }

    return NULL;
}

void cleanup(void) {
    printf("Will perform cleanup...\n");

    arena_cleanup();

    printf("Cleanup completed\n");
}

void create_new_thread(pthread_t* id, void*(*start_routine)(void*), pthread_attr_t* attr) {
    if (id == NULL && start_routine == NULL) {
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    int status = 0;

    if ((status = pthread_create(id, attr, start_routine, NULL)) != 0) {
        fprintf(stderr, "Couldn't create new thread, returned %d\n", status);
        exit(EXIT_FAILURE);
    }
}

void exit_program(int sig) {
    exit(EXIT_SUCCESS);
}

void exit_thread(int sig) {
    pthread_exit(EXIT_SUCCESS);
}

int main(void) {
    srandom(time(NULL));

    arena_id = arena_allocate(10000);

    signal(SIGINT, exit_program);
    signal(SIGUSR1, exit_thread);

    atexit(cleanup);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    create_new_thread(&thread_id_1, thread_routine_1, &attr);
    create_new_thread(&thread_id_2, thread_routine_2, &attr);

    pthread_attr_destroy(&attr);

    while (1);

    return 0;
}