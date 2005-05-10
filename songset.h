/*
 $Id$

 jinamp: a command line music shuffler
 Copyright (C) 2001-2005  Bruce Merry.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 Note: ONLY version 2 of the GPL is allowed. If the FSF release a later version I will
 decide whether to relicense the software under it.
 */

/*! \file set.h
 * \brief set management
 * This file declares all the set management code implemented in
 * songset.c (creation, search, iteration, intersection etc).
 */

#ifndef JINAMP_SONGSET_H
#define JINAMP_SONGSET_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stddef.h>

struct song
{
    /* Public */
    char *name;                                /* filename; read-only */
    struct song *prev, *next;                  /* linked ring for order */
    int order;                                 /* used internally by shuffler */

    /* Private stuff for AVL tree */
    struct song *children[2];          /* 0 is left, 1 is right */
    int depth;
};

struct songset
{
    struct song *root;
    struct song *head;
};

/* allocates and returns an empty set */
struct songset *set_alloc(void);

/* initialises a set to empty */
void set_init(struct songset *set);

/* returns number of elements in set */
size_t set_size(const struct songset *set);

/* True if the set is empty */
int set_empty(const struct songset *set);

/* returns true for successful insert, false for duplicate (but value is still set) */
struct song *set_insert(struct songset *set, const struct song *song);

/* returns true if item found and removed, false if not found */
int set_remove(struct songset *set, const char *item);

/* returns true iff item is in l */
int set_find(const struct songset *set, const char *item);

/* frees the set */
void set_dispose(struct songset *set);

/* calls set_dispose then frees the set structure itself */
void set_free(struct songset *set);

/* passes data to walker for every item in the set. value may be changed */
void set_walk(struct songset *set, void (*walker)(struct song *song, void *data), void *data);

/* merges set2 into set1 */
void set_merge(struct songset *set1, const struct songset *set2);

/* takes everything in set2 and removes it from set1 */
void set_subtract(struct songset *set1, const struct songset *set2);

/* removes anything from set1 not in set2 */
void set_mask(struct songset *set1, const struct songset *set2);

/* sorts set by the 'order' field */
void set_sort(struct songset *set);

#endif /* JINAMP_SET_H */