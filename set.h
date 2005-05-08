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
 * This file declares all the set management code implemented in set.c (creation,
 * search, iteration, intersection etc).
 */

#ifndef JINAMP_SET_H
#define JINAMP_SET_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stddef.h>

typedef struct node_s
{
    char *key;
    void *value;
    struct node_s *children[2];          /* 0 is left, 1 is right */
    int depth;
} node;

typedef struct
{
    node *head;
    int owns_values;
} set;

/* allocates and returns an empty set */
set *set_alloc(int owns_values);

/* initialises a set to empty */
void set_init(set *l, int owns_values);

/* returns number of elements in set */
size_t set_count(const set *l);

/* returns true for successful insert, false for duplicate (but value is still set) */
int set_insert(set *l, const char *item, void *value);

/* returns true if item found and removed, false if not found */
int set_remove(set *l, const char *item);

/* returns true iff item is in l */
int set_find(const set *l, const char *item);

/* frees the set */
void set_dispose(set *l);

/* calls set_dispose then frees the set structure itself */
void set_free(set *l);

/* passes data to walker for every item in the set. value may be changed */
void set_walk(set *l, void (*walker)(const char *item, void **value, void *data), void *data);

/* merges set2 into set1 */
void set_merge(set *set1, const set *set2);

/* takes everything in set2 and removes it from set1 */
void set_subtract(set *set1, const set *set2);

/* removes anything from set1 not in set2 */
void set_mask(set *set1, const set *set2);

#endif /* JINAMP_SET_H */
