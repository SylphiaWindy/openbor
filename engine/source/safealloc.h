/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef SAFEALLOC_H
#define SAFEALLOC_H

#include <stdlib.h>
#include <string.h> // for strlen
#include "utils.h" // for checkAlloc
#undef strdup

#define MALLOCLIKE __attribute__((__malloc__))

#ifdef BOR_PROF
/*
 * Allocation counters.
 *
 * Tearing a level down frees ~765k objects, and an instruction pool made that
 * 20x cheaper by not going to the allocator at all.  The build-up side has
 * never been measured: this says how much of parsing and compiling is the
 * allocator rather than the parser.  Counting is nearly free; the timing costs
 * a clock read per allocation, so read count and time separately.
 */
#include <stdint.h>
extern uint64_t bor_alloc_count;
extern uint64_t bor_alloc_bytes;
extern uint64_t bor_alloc_us;
uint64_t prof_us(void);
/* Charge an allocation to the line that asked for it, so the sites worth
   pooling can be read off instead of guessed at. */
void bor_alloc_site(const char *file, int line, uint64_t bytes, uint64_t us);
#define BOR_ALLOC_NOTE(sz, expr)                        \
    ({ uint64_t _t0 = prof_us(); void *_p = (expr);     \
       uint64_t _d = prof_us() - _t0;                   \
       bor_alloc_us += _d;                              \
       bor_alloc_count++; bor_alloc_bytes += (uint64_t)(sz); \
       bor_alloc_site(file, line, (uint64_t)(sz), _d); _p; })
#else
#define BOR_ALLOC_NOTE(sz, expr) (expr)
#endif

static inline void *safeRealloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(BOR_ALLOC_NOTE(size, realloc(ptr, size)), size, func, file, line);
}

// attributes can only be declared on function declarations, so declare these before defining them
static inline void *safeMalloc(size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeStrdup(const char *str, const char *func, const char *file, int line) MALLOCLIKE;

static inline void *safeMalloc(size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(BOR_ALLOC_NOTE(size, malloc(size)), size, func, file, line);
}

static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(BOR_ALLOC_NOTE((uint64_t)nmemb * size, calloc(nmemb, size)), size, func, file, line);
}

static inline void *safeStrdup(const char *str, const char *func, const char *file, int line)
{
    // reimplement strdup to avoid doing strlen(str) twice
    int size = strlen(str) + 1;
    char *newString = (char *) checkAlloc(BOR_ALLOC_NOTE(size, malloc(size)), size, func, file, line);
    memcpy(newString, str, size);
    return newString;
}

#define realloc(ptr, size) safeRealloc(ptr, size, __func__, __FILE__, __LINE__)
#define malloc(size) safeMalloc(size, __func__, __FILE__, __LINE__)
#define calloc(nmemb, size) safeCalloc(nmemb, size, __func__, __FILE__, __LINE__)
#define strdup(str) safeStrdup(str, __func__, __FILE__, __LINE__)

#endif // !defined SAFEALLOC_H
