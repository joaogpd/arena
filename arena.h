#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_ARENAS 10
#define DEBUG

typedef struct arena Arena;
typedef int32_t arena_t; 

// Creates an arena of 'bytes' size, and returns a unique identifier.
// On error, it returns -1.
arena_t arena_allocate(size_t bytes);

// Deallocates a previously allocated arena. Returns -1 on error.
int arena_deallocate(arena_t id);

// Requests a memory address of 'bytes' size from the arena specified by the 'id'.
// Returns NULL on error.
void* arena_request_memory(arena_t id, size_t bytes);

// Frees a memory address from a given arena. Returns -1 on error.
int arena_free_memory(arena_t id, void* memaddr);

// Cleanup of all arenas at the end of execution. Will invalidate any pointers 
// returned from 'arena_request_memory'.
void arena_cleanup(void);

#endif