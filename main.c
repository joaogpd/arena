#include "arena.h"
#include <stdio.h>

int main(void) {
    arena_t arena_1 = arena_allocate(1000);

    printf("Arena id: %d\n", arena_1);

    int* int_buffer = (int*)arena_request_memory(arena_1, 10);
    
    printf("Buffer at first: %p\n", int_buffer);

    for (int i = 0; i < 10; i++) {
        int_buffer[i] = i;
    }

    for (int i = 0; i < 10; i++) {
        printf("esperado: %d | real: %d\n", i, int_buffer[i]);
    }

    int status = arena_free_memory(arena_1, int_buffer);

    printf("Free memory status: %d\n", status);

    int_buffer = (int*)arena_request_memory(arena_1, 1000);

    printf("Buffer later: %p\n", int_buffer);

    arena_deallocate(arena_1);

    arena_cleanup();

    return 0;
}