/* This file is a part of the leakchk library.
 * Copyright (C) 2022 by Sergey Lafin
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory. */

#include "leakchk.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <csl/standard.h>
#include <csl/platform.h>
#include <csl/c89.h>
#include <csl/die.h>

/* disables critical section checking */
#ifndef LC_DISABLE_THREADSAFETY
#   define lc_thrdsafe 1
#   ifndef CSL_PLATFORM_POSIX
#       error "thread safe leakchk only supports POSIX-compliant platforms"
#   else 
#       include <pthread.h>
#   endif
#endif

/* disables location tracking, see struct lc_block definition */
#ifndef LC_DISABLE_LOCATIONTRACKING
#   define lc_loctrack 1
#endif

/* disables magic number checks in lc_dofree and lc_dorealloc */
#ifndef LC_DISABLE_MAGICCHECK
#   define LC_MAGIC uint32_t magic;
#   define lc_magichk 1
#else
#   define LC_MAGIC 
#endif

/* don't save function names */
#ifndef LC_DISABLE_FUNCNAME
#   define lc_trackfunc 1
#endif

#define LC_MAGICNUMBER 0x534C4C43

/* this struct will prepend the actual allocation */
struct lc_block {
    size_t size;

#if lc_loctrack

#   if lc_trackfunc
        /* the function the allocation's happened in */
        const char *function;
#   endif

    const char *file;
    unsigned    line;
#endif

    struct lc_block *prev,
                    *next;

    LC_MAGIC
};

/* mutex for critical areas of the code */
#if lc_thrdsafe
    static pthread_mutex_t mtx;
#   define lc_lockmtx(mtx) \
        pthread_mutex_lock (mtx)
#   define lc_unlockmtx(mtx) \
        pthread_mutex_unlock (mtx)
#   define lc_destroymtx(mtx) \
        pthread_mutex_destroy (mtx)

#   define LC_BEGINCRIT \
        pthread_mutex_lock (&mtx);
#   define LC_ENDCRIT \
        pthread_mutex_unlock (&mtx);
#else
#   define lc_lockmtx(mtx)
#   define lc_unlockmtx(mtx)
#   define lc_destroymtx(mtx)

#   define LC_BEGINCRIT
#   define LC_ENDCRIT
#endif

#define LC_PTROF(x) \
    (void *)((char *)(x) + sizeof (struct lc_block))

#define LC_BLKOF(x) \
    (void *)((char *)(x) - sizeof (struct lc_block))

static size_t blk_count  = 0;
static struct lc_block *head_blk = NULL,
                       *last_blk = NULL;

int lc_init (void) {
#if lc_thrdsafe
    int result = pthread_mutex_init(&mtx, NULL);
    if (result) {
        return LC_PTHREAD_ERROR;
    }
#endif

    return 0;
}

void lc_deinit (void) {
    if (blk_count) {
        fprintf (stderr, "LEAKCHK: %lu tracked allocations have not been freed:\n", blk_count);
        lc_printblocks (stderr);
    }

    lc_destroymtx (&mtx);
}

CSL_INLINE void add_block (struct lc_block *blk) {
    ++blk_count;

    if (!head_blk) {
        head_blk = blk;
    } else {
        last_blk->next = blk;
        blk->prev = last_blk;
    }

    last_blk  = blk;
}

CSL_INLINE struct lc_block *fill_block (struct lc_block *blk, size_t size, const char *function, const char *file, unsigned line) {
    blk->size = size;

#if lc_loctrack
    blk->file = file;
    blk->line = line;
#   if lc_trackfunc
        blk->function = function;
#   endif
#endif

#if lc_magichk
    blk->magic = LC_MAGICNUMBER;
#endif

    blk->next = NULL;

    return blk;
}

#define LC_BLOCKSZ \
    sizeof (struct lc_block)

/* allocate a new lc_block */
void *lc_domalloc (size_t size, const char *function, const char *file, unsigned line) {
    LC_BEGINCRIT

    struct lc_block *blk = malloc (LC_BLOCKSZ + size);

    if (!blk) {
        return NULL;
    }

    fill_block (blk, size, function, file, line);
    add_block (blk);

    LC_ENDCRIT

    return LC_PTROF (blk);
}

void *lc_docalloc (size_t size, const char *function, const char *file, unsigned line) {
    LC_BEGINCRIT

    struct lc_block *blk = malloc (LC_BLOCKSZ + size);
    if (!blk) {
        return NULL;
    }


    fill_block (blk, size, function, file, line);
    add_block (blk);

    void *ptr = LC_PTROF (blk);
    memset (ptr, '\0', size);

    LC_ENDCRIT

    return ptr;
}

#define lc_checkmagic(ptr) \
    (*(uint32_t *)((char *)ptr - LC_BLOCKSZ + offsetof (struct lc_block, magic)) == LC_MAGICNUMBER)

void *lc_dorealloc (void *ptr, size_t size, const char *function, const char *file, unsigned line) {
    if (!lc_checkmagic (ptr)) {
        csl_die ("lc_realloc: trying to realloc a block which was not allocated with an lc_* function");
    }

    LC_BEGINCRIT

    struct lc_block *pblk = LC_BLKOF (ptr), *nblk;

    struct lc_block *next = pblk->next,
                    *prev = pblk->prev;

    nblk = realloc (pblk, LC_BLOCKSZ + size);

    if (pblk == head_blk)
        head_blk = nblk;
    if (pblk == last_blk)
        last_blk = last_blk->prev;
    if (!nblk)
        return NULL;

    nblk->size = size;

    if (nblk == pblk) {
        return ptr;
    }

    if (next)
        next->prev = nblk;
    if (prev)
        prev->next = nblk;

    LC_ENDCRIT

    return LC_PTROF (nblk);
}

void lc_dofree (void *ptr) {
    if (!lc_checkmagic (ptr)) {
        csl_die ("lc_free: trying to free a block which was not allocated with an lc_* function");
    }

    LC_BEGINCRIT

    struct lc_block *blk = LC_BLKOF (ptr);

    if (blk->prev) {
        blk->prev->next = blk->next;
    } else {
        head_blk = blk->next;
        blk->next->prev = NULL;
    }
    --blk_count;

    free (blk);

    LC_ENDCRIT
}

struct lc_block *lc_getallocations (void) {
    return head_blk;
}

size_t lc_countallocations (void) {
    return blk_count;
}

struct lc_block *lc_nextblock (struct lc_block *blk) {
    return blk->next ? blk->next : NULL;
}

#define LC_ADDRESSFORMAT \
    "0x%lX"
#define LC_ADDROF(x) \
    *(unsigned long *)&x

void lc_printblock (FILE *fd, struct lc_block *blk) {
    fprintf (fd, "LC_BLOCK @" LC_ADDRESSFORMAT ":\n"
                 "\tsize: %lu"
#if lc_loctrack
                ", allocated in %s:%u"
#   if lc_trackfunc
                " (function %s)"
#   endif
#endif
                "\n", LC_ADDROF (blk), blk->size
#if lc_loctrack
                , blk->file, blk->line 
#   if lc_trackfunc
                , blk->function
#   endif
#endif
                );
}

void lc_printblocks (FILE *fd) {
    LC_BEGINCRIT

    struct lc_block *alloc = head_blk;

    while (alloc) {
        lc_printblock (fd, alloc);
        alloc = lc_nextblock (alloc);
    }

    LC_ENDCRIT
}
