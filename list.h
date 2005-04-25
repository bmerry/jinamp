/*
 $Id: list.h,v 1.6 2005/04/25 15:16:31 bruce Exp $

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

/*! \file list.h
 * \brief set management
 * This file declares all the set management code implemented in list.c (creation,
 * search, iteration, intersection etc). A "list" is actually a set (it's called a
 * list because that was the original implementation).
 */

#ifndef JINAMP_LIST_H
#define JINAMP_LIST_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

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
} list;

/* allocates and returns an empty list */
list *list_alloc(int owns_values);

/* initialises a list to empty */
void list_init(list *l, int owns_values);

/* returns number of elements in list */
size_t list_count(const list *l);

/* returns true for successful insert, false for duplicate */
int list_insert(list *l, const char *item, void *value);

/* returns true if item found and removed, false if not found */
int list_remove(list *l, const char *item);

/* returns true iff item is in l */
int list_find(const list *l, const char *item);

/* frees the list */
void list_dispose(list *l);

/* calls list_dispose then frees the list structure itself */
void list_free(list *l);

/* passes data to walker for every item in the list */
void list_walk(const list *l, void (*walker)(const char *item, void *value, void *data), void *data);

/* merges list2 into list1 */
void list_merge(list *list1, const list *list2);

/* takes everything in list2 and removes it from list1 */
void list_subtract(list *list1, const list *list2);

/* removes anything from list1 not in list2 */
void list_mask(list *list1, const list *list2);

#endif /* JINAMP_LIST_H */
