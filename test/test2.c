#include "../arena.h"
#include <stdio.h>

int main(void) {
    arena_t arena_ids[MAX_ARENAS] = {0};

    for (int i = 0; i < MAX_ARENAS; i++) {
        arena_ids[i] = arena_allocate(1000);
    }

    for (int i = 0; i < MAX_ARENAS; i++) {
        printf("Arena %d id: %d\n", i, arena_ids[i]);
    }

    printf("New arena: %d\n", arena_allocate(1000));

    arena_deallocate(arena_ids[3]);

    printf("New arena (after deallocate): %d\n", arena_allocate(1000));
    
    printf("New arena: %d\n", arena_allocate(1000));

    arena_cleanup();

    return 0;
}