// Arenas: https://nullprogram.com/blog/2023/09/27/
// Lists:  https://nullprogram.com/blog/2023/10/05/

#pragma once

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

#include "types.h"

#define sizeof(x)    (ptrdiff_t)sizeof(x)
#define countof(a)   (sizeof(a) / sizeof(*(a)))
#define lengthof(s)  (countof(s) - 1)

#define arena_alloc(...)            arena_allocx(__VA_ARGS__,arena_alloc4,arena_alloc3,arena_alloc2)(__VA_ARGS__)
#define arena_allocx(a,b,c,d,e,...) e
#define arena_alloc2(a, t)          (t *)arena_alloc_allign(a, sizeof(t), alignof(t), 1, 0)
#define arena_alloc3(a, t, n)       (t *)arena_alloc_allign(a, sizeof(t), alignof(t), n, 0)
#define arena_alloc4(a, t, n, f)    (t *)arena_alloc_allign(a, sizeof(t), alignof(t), n, f)

typedef struct {
    void *base;
    void *start;
    void *end;
} Arena;

Arena arena_new(usize capacity);
void *arena_alloc_allign(Arena *arena, usize size, usize align, u32 count, u8 flags);
void arena_free(Arena *arena);
void arena_reset(Arena *arena);
void list_grow(void *slice, ptrdiff_t size, Arena *arena);

#define list_push(slice, arena) \
    ((slice)->length >= (slice)->capacity \
        ? list_grow(slice, sizeof(*(slice)->data), arena), \
          (slice)->data + (slice)->length++ \
        : (slice)->data + (slice)->length++)

#ifdef ARENA_IMPLEMENTATION

Arena arena_new(usize capacity) {
    Arena a = { 0 };
    a.base = malloc(capacity);
    a.start = a.base;
    a.end = a.start ? a.start + capacity : 0;
    return a;
}

void *arena_alloc_allign(Arena *arena, usize size, usize align, u32 count, u8 flags) {
    ptrdiff_t padding = -(uptr) arena->start & (align - 1);
    ptrdiff_t available = arena->end - arena->start - padding;
    if (available < 0 || count > available / size) { raise(SIGTRAP); }

    void *p = arena->start + padding;
    arena->start += padding + count * size;
    return memset(p, 0, count * size);
}

void arena_free(Arena *arena) {
    free(arena->base);
    arena->base = arena->start = arena->end = NULL;
}

void arena_reset(Arena *arena) {
    arena->start = arena->base;
}


void list_grow(void *slice, ptrdiff_t size, Arena *arena) {
    struct {
        void     *data;
        ptrdiff_t length;
        ptrdiff_t capacity;
    } replica;
    memcpy(&replica, slice, sizeof(replica));

    replica.capacity = replica.capacity ? replica.capacity : 1;
    ptrdiff_t align = 16;
    void *data = arena_alloc_allign(arena, 2*size, align, replica.capacity, 0);
    replica.capacity *= 2;

    if (replica.length) { memcpy(data, replica.data, size*replica.length); }
    replica.data = data;
    memcpy(slice, &replica, sizeof(replica));
}

#endif
