#include "arena.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

typedef struct arena {
    arena_t a_id;
    void* a_start_memaddr;
    void* a_curr_memaddr;
    uint32_t a_size;
    uint32_t a_notinuse_size;
} Arena;

typedef struct memory_chunk {
    uint8_t mc_arena_id;
    void* mc_memaddr;
    size_t mc_size;
    bool mc_available;
    struct memory_chunk* next;
} MemoryChunk;

arena_t current_arena_id = 0;

pthread_mutex_t current_arena_id_mutex = PTHREAD_MUTEX_INITIALIZER;

bool arena_initialized = false;

pthread_mutex_t arena_initialized_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t prevent_id_race_condition = PTHREAD_MUTEX_INITIALIZER;

Arena* open_arenas[MAX_ARENAS] = {NULL};
MemoryChunk* allocated_memory_chunks[MAX_ARENAS] = {NULL};

pthread_mutex_t open_arenas_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t allocated_memory_chunks_mutex[MAX_ARENAS];

static void arena_mutex_initialize(void) {
    for (int i = 0; i < MAX_ARENAS; i++) {
        pthread_mutex_init(&(allocated_memory_chunks_mutex[i]), NULL);
    }
}

// Perform any necessary initialization before allocating any arenas.
static void arena_initialize(void) {
    arena_mutex_initialize();
}

static arena_t arena_find_available_id(void) {
    pthread_mutex_lock(&current_arena_id_mutex);
    if (current_arena_id < MAX_ARENAS) {
        arena_t temp_id = current_arena_id;
        current_arena_id++;
        pthread_mutex_unlock(&current_arena_id_mutex);
        return temp_id;
    }
    pthread_mutex_unlock(&current_arena_id_mutex);

    pthread_mutex_lock(&open_arenas_mutex);
    for (int i = 0; i < MAX_ARENAS; i++) {
        if (open_arenas[i] == NULL) {
            pthread_mutex_unlock(&open_arenas_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&open_arenas_mutex);

    return -1; // no arena spots available
}

arena_t arena_allocate(size_t bytes) {
    pthread_mutex_lock(&arena_initialized_mutex);
    if (!arena_initialized) {
        arena_initialize();
    }
    pthread_mutex_unlock(&arena_initialized_mutex);

    pthread_mutex_lock(&prevent_id_race_condition);
    arena_t new_id = arena_find_available_id();

    if (new_id == -1) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: can't have more than %d arenas\n", MAX_ARENAS);
#endif
        pthread_mutex_unlock(&prevent_id_race_condition);
        return -1;
    }

    Arena* new_arena = (Arena*)malloc(sizeof(Arena));

    if (new_arena == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: couldn't allocate arena structure\n");
#endif
        return -1;
    }

    new_arena->a_start_memaddr = malloc(bytes);

    if (new_arena->a_start_memaddr == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: couldn't allocate arena memory\n");
#endif
        free(new_arena);
        return -1;
    }

    new_arena->a_curr_memaddr = new_arena->a_start_memaddr;
    new_arena->a_size = bytes;
    new_arena->a_notinuse_size = bytes;
    new_arena->a_id = new_id;

    pthread_mutex_lock(&open_arenas_mutex);
    open_arenas[new_arena->a_id] = new_arena;
    pthread_mutex_unlock(&open_arenas_mutex);

    pthread_mutex_unlock(&prevent_id_race_condition);

    return new_arena->a_id;
}

static void arena_free_arena_structure(Arena* arena) {
    if (arena != NULL) {
        free(arena->a_start_memaddr);
    }
    free(arena);
}

int arena_deallocate(arena_t id) {
    if (id > MAX_ARENAS) {
#ifdef DEBUG
        fprintf(stderr, "Can't free invalid arena\n");
#endif
        return -1;
    }

    pthread_mutex_lock(&open_arenas_mutex);

    Arena* arena = open_arenas[id];

    if (arena == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: arena requested to be deallocated has not been allocated yet\n");
#endif
        return -1;
    }

    arena_free_arena_structure(arena);

    open_arenas[id] = NULL;

    pthread_mutex_unlock(&open_arenas_mutex);

    // if arena ids were to be recycled, this would need to be done differently
    pthread_mutex_lock(&(allocated_memory_chunks_mutex[id]));
    MemoryChunk* mem_chunk = allocated_memory_chunks[id];
    while (mem_chunk != NULL) {
        MemoryChunk* temp = mem_chunk->next;
        free(mem_chunk);
        mem_chunk = temp;
    }
    allocated_memory_chunks[id] = NULL;
    pthread_mutex_unlock(&(allocated_memory_chunks_mutex[id]));

    return 0;
}

void* arena_request_memory(arena_t id, size_t bytes) {
    if (id > MAX_ARENAS) {
        return NULL;
    }

    if (bytes <= 0) {
        return NULL;
    }

    // first look through MemoryChunk linked list
    pthread_mutex_lock(&(allocated_memory_chunks_mutex[id]));
    MemoryChunk* mem_chunk = allocated_memory_chunks[id];
    while (mem_chunk != NULL) {
        if (mem_chunk->mc_available && mem_chunk->mc_size >= bytes) {
            mem_chunk->mc_available = false;
            pthread_mutex_unlock(&(allocated_memory_chunks_mutex[id]));
            return mem_chunk->mc_memaddr;
        } 
        mem_chunk = mem_chunk->next;
    }
    pthread_mutex_unlock(&(allocated_memory_chunks_mutex[id]));

    MemoryChunk* new_mem_chunk = (MemoryChunk*)malloc(sizeof(MemoryChunk));
    if (new_mem_chunk == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: couldn't allocate new memory chunk\n");
#endif
        return NULL;
    }

    pthread_mutex_lock(&open_arenas_mutex);
    Arena* arena = open_arenas[id];

    if (arena == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: couldn't find arena by id\n");
#endif
        free(new_mem_chunk);
        pthread_mutex_unlock(&open_arenas_mutex);
        return NULL;
    }

    // verify if arena has available size
    if (arena->a_notinuse_size <= bytes) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: not enough memory in arena for new allocation\n");
#endif
        free(new_mem_chunk);
        pthread_mutex_unlock(&open_arenas_mutex);
        return NULL;
    }

    void* address = arena->a_curr_memaddr;

    arena->a_curr_memaddr = arena->a_curr_memaddr + bytes;
    arena->a_notinuse_size -= bytes;

    pthread_mutex_unlock(&open_arenas_mutex);

    new_mem_chunk->mc_arena_id = id;
    new_mem_chunk->mc_available = false;
    new_mem_chunk->mc_memaddr = address;
    new_mem_chunk->mc_size = bytes;
    pthread_mutex_lock(&(allocated_memory_chunks_mutex[id]));
    new_mem_chunk->next = allocated_memory_chunks[id];
    allocated_memory_chunks[id] = new_mem_chunk;
    pthread_mutex_unlock(&(allocated_memory_chunks_mutex[id]));

    return new_mem_chunk->mc_memaddr;
}

int arena_free_memory(arena_t id, void* memaddr) {
    if (id > MAX_ARENAS) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: invalid arena identifier\n");
#endif
        return -1;
    }

    pthread_mutex_lock(&(allocated_memory_chunks_mutex[id]));
    MemoryChunk* mem_chunk = allocated_memory_chunks[id];
    while (mem_chunk != NULL) {
        if (mem_chunk->mc_memaddr == memaddr) {
            mem_chunk->mc_available = true;
            break;
        }
        mem_chunk = mem_chunk->next;
    }
    pthread_mutex_unlock(&(allocated_memory_chunks_mutex[id]));

    if (mem_chunk == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ERROR: this address does not seem to belong to this arena\n");
#endif
        return -1;
    }

    return 0;
}

void arena_cleanup(void) {
    pthread_mutex_lock(&open_arenas_mutex);

    for (int i = 0; i < MAX_ARENAS; i++) {
        if (open_arenas[i] != NULL) {
#ifdef DEBUG
            printf("Freed arena: %d\n", i);
#endif
            arena_free_arena_structure(open_arenas[i]);
        } 
    }

    pthread_mutex_unlock(&open_arenas_mutex);

    for (int i = 0; i < MAX_ARENAS; i++) {
        pthread_mutex_lock(&(allocated_memory_chunks_mutex[i]));
        MemoryChunk* mem_chunk = allocated_memory_chunks[i];
        int counter = 0;
        while (mem_chunk != NULL) {
            MemoryChunk* temp = mem_chunk->next;
            free(mem_chunk);
#ifdef DEBUG
            printf("Freed memory chunk %d of arena %d\n", counter, i);
#endif
            mem_chunk = temp;
        }
        pthread_mutex_unlock(&(allocated_memory_chunks_mutex[i]));
    }
}