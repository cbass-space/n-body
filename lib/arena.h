#pragma once

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include "types.h"

// https://nullprogram.com/blog/2023/09/27/

#define sizeof(x)    (ptrdiff_t)sizeof(x)
#define countof(a)   (sizeof(a) / sizeof(*(a)))
#define lengthof(s)  (countof(s) - 1)

typedef struct {
    void *base;
    void *start;
    void *end;
} Arena;

Arena arena_new(usize capacity) {
    Arena a = {0};
    a.base = malloc(capacity);
    a.start = a.base;
    a.end = a.start ? a.start+capacity : 0;
    return a;
}

#define arena_alloc(...)            arena_allocx(__VA_ARGS__,arena_alloc4,arena_alloc3,arena_alloc2)(__VA_ARGS__)
#define arena_allocx(a,b,c,d,e,...) e
#define arena_alloc2(a, t)          (t *)arena_alloc_allign(a, sizeof(t), alignof(t), 1, 0)
#define arena_alloc3(a, t, n)       (t *)arena_alloc_allign(a, sizeof(t), alignof(t), n, 0)
#define arena_alloc4(a, t, n, f)    (t *)arena_alloc_allign(a, sizeof(t), alignof(t), n, f)

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

