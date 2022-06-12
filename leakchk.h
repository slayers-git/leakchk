/* leakchk - simple allocation tracker and leak checker for C/C++.
 * Copyright (C) 2022 Sergey Lafin
 *
 * This library is free software; you can redistribute it and/or
 * odify it under the terms of the GNU Lesser General Public
 * icense as published by the Free Software Foundation; either
 * ersion 2.1 of the License, or (at your option) any later version.
 * 
 * his library is distributed in the hope that it will be useful,
 * ut WITHOUT ANY WARRANTY; without even the implied warranty of
 * ERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * esser General Public License for more details.
 * 
 * ou should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef __LEAKCHK_H__
#define __LEAKCHK_H__

#include <stddef.h>
#include <csl/platform.h>
#include <csl/begin_code.h>

#include <stdio.h>

#if defined (CSL_PLATFORM_MSVC) || defined (CSL_PLATFORM_GCC)
#   define LC_FUNCTION __FUNCTION__
#elif defined (CSL_STANDARD_CPP)
#   define LC_FUNCTION __func__
#else
#   error "Undefined __FUNCTION__ macro"
#endif

#if defined(_WIN32) || defined(__WIN32__)
#	if defined(leakchk_EXPORTS)
#		define  LC_API __declspec(dllexport)
#	else
#		define  LC_API __declspec(dllimport)
#	endif
#else
#	define LC_API
#endif

enum lc_error {
    LC_OK = 0,
    LC_PTHREAD_ERROR = 1,
};

CSL_BEGIN_CODE

/* initialize the library */
int lc_init (void);
/* de-initialize the library */
void lc_deinit (void);

#define lc_malloc(size) \
    (lc_domalloc ((size), LC_FUNCTION, __FILE__, __LINE__))

#define lc_calloc(nmemb, size) \
    (lc_docalloc ((nmemb) * (size), LC_FUNCTION, __FILE__, __LINE__))

#define lc_realloc(pblk, size) \
    (lc_dorealloc ((pblk), (size), LC_FUNCTION, __FILE__, __LINE__))

#define lc_free(blk) \
    (lc_dofree (blk))

#ifdef LC_REPLACESTDLIB
#   define malloc lc_malloc 
#   define calloc lc_calloc 
#   define realloc lc_realloc
#   define free lc_free
#endif

/* get the SLL of allocations in a form of an opaque struct */
struct lc_block *lc_getallocations (void);
/* traverse to the next allocation */
struct lc_block *lc_nextblock (struct lc_block *blk);
/* get the amount of tracked allocations */
size_t lc_countallocations (void);

/* print the information of a given allocation to stream */
void lc_printblock (FILE *fd, struct lc_block *);

/* print all tracked allocations */
void lc_printblocks (FILE *fd);

/****************/
/* INTERNAL USE */
/****************/

#define LC_COMMONPROTO \
    const char *function, const char *file, unsigned line

void *lc_domalloc  (size_t size, LC_COMMONPROTO);
void *lc_docalloc  (size_t size, LC_COMMONPROTO);
void *lc_dorealloc (void *pblk, size_t size, LC_COMMONPROTO);

#undef LC_COMMONPROTO

void lc_dofree (void *blk);

CSL_END_CODE

#endif
