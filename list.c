/*
 $Id: list.c,v 1.3 2002/02/26 11:46:05 bruce Exp $

 jinamp: a command line music shuffler
 Copyright (C) 2001  Bruce Merry.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <list.h>
#include <misc.h>
#include <debug.h>

#define GET_DEPTH(node) (((node) ? (node)->depth : -1))
#define TOO_SHALLOW(node, side) (GET_DEPTH((node)->children[(side)]) < (node)->depth - 2)

list *list_alloc() {
  list *tmp = (list *) safe_malloc(sizeof(list));

  dprintf(DBG_LIST_OPS, "%p: list created\n", tmp);
  list_init(tmp);
  return tmp;
}

void list_init(list *l) {
  dprintf(DBG_LIST_OPS, "%p: list initialised\n", l);
  l->head = NULL;
}

void count_walker(char *item, void *data) {
  (*((size_t *) data))++;
}

size_t list_count(list *l) {
  size_t tot = 0;
  list_walk(l, count_walker, (void *) &tot);
  dprintf(DBG_LIST_OPS, "%p: counted: %d elements\n", (int) tot);
  return tot;
}

void avl_fix_depth(node *n) {
  int i;

  n->depth = 0;
  for (i = 0; i < 2; i++)
    if (n->children[i] && n->children[i]->depth >= n->depth)
      n->depth = n->children[i]->depth + 1;
}

#if DEBUG

/* checks that the tree is properly sorted w/o duplication, and returns the
 * first and last string. root must exist.
 */
void avl_assert_order(node *root, char **first, char **last) {
  char *tmp;

  *first = *last = NULL;
  if (root->children[0]) {
    avl_assert_order(root->children[0], first, &tmp);
    if (strcmp(root->item, tmp) <= 0) {
      if (strcmp(root->item, tmp) == 0)
        die("Duplicate item: %s\n", root->item);
      else
        die("Out of order: %s before %s\n", tmp, root->item);
    }
  }
  if (root->children[1]) {
    avl_assert_order(root->children[1], &tmp, last);
    if (strcmp(root->item, tmp) >= 0) {
      if (strcmp(root->item, tmp) == 0)
        die("Duplicate item: %s\n", root->item);
      else
        die("Out of order: %s before %s\n", root->item, tmp);
    }
  }
  if (!(*first)) *first = root->item;
  if (!(*last)) *last = root->item;
}

void avl_assert_depth(node *root) {
  int old;
  int i;

  for (i = 0; i < 2; i++)
    if (root->children[i]) avl_assert_depth(root->children[i]);
  old = root->depth;
  avl_fix_depth(root);
  if (old != root->depth)
    die("assert_depth failed on node %s: wrong depth count", root->item);
  for (i = 0; i < 2; i++)
    if (TOO_SHALLOW(root, i))
      die("assert_depth failed on node %s: not balanced", root->item);
}

void avl_print_tree(node *root, int offset) {
  if (root->children[0]) avl_print_tree(root->children[0], offset + 3);
  dprintf(DBG_LIST_SHOW, "%*c%s\n", offset, ' ', root->item);
  if (root->children[1]) avl_print_tree(root->children[1], offset + 3);
}

/* checks that the AVL tree is valid */
void avl_assert_valid(list *l) {
  char *first, *last;                       /* dummy */
  dprintf(DBG_LIST_OPS, "%p: asserting AVL tree\n");
  avl_assert_order(l->head, &first, &last);
  avl_assert_depth(l->head);
}

void print_list(list *l) {
  dprintf(DBG_LIST_SHOW, "Printing list %p\n", l);
  if (debug_flags && DBG_LIST_SHOW && l->head)
    avl_print_tree(l->head, 2);
}
#else /* DEBUG */
#define avl_assert_valid(root)
#define print_list(l)
#endif /* DEBUG */

/* child is the side that is too deep */
void avl_single_rotate(node **root, int child) {
  node *n, *tmp;

  n = *root;
  tmp = n->children[child];
  n->children[child] = tmp->children[!child];
  tmp->children[!child] = n;
  *root = tmp;
  avl_fix_depth(n);
  avl_fix_depth(tmp);
}

/* child is the side that is too deep */
void avl_double_rotate(node **root, int child) {
  node *n, *tmp;

  n = *root;
  tmp = n->children[child];
  *root = tmp->children[!child];
  tmp->children[!child] = (*root)->children[child];
  n->children[child] = (*root)->children[!child];
  (*root)->children[child] = tmp;
  (*root)->children[!child] = n;
  avl_fix_depth(tmp);
  avl_fix_depth(n);
  avl_fix_depth(*root);
}

/* Inserts an item into a subtree, balancing if necessary. A return of 2 indicates
 * successful insertion and balancing was done, 1 indicates insertion w/o balancing
 * and 0 indicates that the item was a duplicate
 */
int avl_insert(node **root, char *item) {
  int r, sub;
  int old_depth;
  node *n;
  int child;

  n = *root;
  r = strcmp(item, n->item);
  if (r == 0) return 0;                /* duplicate */
  child = r > 0;
  if (!n->children[child]) {
    n->children[child] = (node *) safe_malloc(sizeof(node));
    n->children[child]->children[0] = n->children[child]->children[1] = NULL;
    n->children[child]->depth = 0;
    n->children[child]->item = duplicate(item);
    if (n->depth == 0) n->depth = 1;
    return 1;
  }
  sub = avl_insert(&n->children[child], item);
  if (sub == 2) return 2;              /* tree has already been balanced */
  else if (sub == 1) {
    old_depth = n->depth;
    avl_fix_depth(n);
    if (TOO_SHALLOW(n, !child)) {
      if (GET_DEPTH(n->children[child]->children[child]) == n->children[child]->depth - 1)
        avl_single_rotate(root, child);
      else
        avl_double_rotate(root, child);
    }
    return (old_depth == n->depth) ? 2 : 1;
  }
  return 0;
}

/* returns true for successful insert, false for duplicate */
int list_insert(list *l, char *item) {
  node *tmp;
  int r;

  dprintf(DBG_LIST_OPS, "%p: inserting %s\n", l, item);
  if (l->head == NULL) {
    tmp = (node *) safe_malloc(sizeof(node));
    tmp->children[0] = NULL;
    tmp->children[1] = NULL;
    tmp->item = duplicate(item);
    tmp->depth = 0;
    l->head = tmp;
    r = 1;
  }
  else
    r = avl_insert(&l->head, item);
  print_list(l);
  avl_assert_valid(l);
  return r;
}

/* return:
 * 0: not found
 * 1: removed
 * 2: removed, no further balancing required
 */
int avl_remove(node **root, char *item, int strings) {
  int cmp;
  int sub;                   /* return code from subcalls */
  int i;
  int old_depth;
  node *tmp, *n;

  n = *root;
  if (!n) return 0;
  cmp = strcmp(item, n->item);
  sub = -1;
  if (cmp == 0) {                      /* found it, now make it a leaf */
    if (strings)
      free(n->item);
    for (i = 0; i < 2; i++)
      if (n->children[i]) {
        tmp = n->children[i];
        while (tmp->children[!i])
          tmp = tmp->children[!i];
        n->item = tmp->item;
        sub = avl_remove(&n->children[i], tmp->item, 0);
        if (!sub) die("avl_remove subcall failed???");
        break;
      }
    if (sub == -1) {
      *root = NULL;
      return 1;
    }
  }
  else {
    i = cmp > 0;
    sub = avl_remove(&n->children[i], item, strings);
  }
  if (sub == 1) {                      /* may need to balance */
    old_depth = n->depth;
    if (TOO_SHALLOW(n, i)) {
      if (GET_DEPTH(n->children[!i]->children[i]) > GET_DEPTH(n->children[i])
          && GET_DEPTH(n->children[!i]->children[!i]) == GET_DEPTH(n->children[i]))
        avl_double_rotate(root, !i);
      else
        avl_single_rotate(root, !i);
    }
    else
      avl_fix_depth(n);
    return (old_depth == n->depth) ? 2 : 1;
  }
  else
    return sub;
}

/* returns true if item found and removed, false if not found */
int list_remove(list *l, char *item) {
  int r;

  dprintf(DBG_LIST_OPS, "%p: removing %s\n", item);
  if (l->head == NULL)
    r = 0;
  else
    r = (avl_remove(&l->head, item, 1) != 0);
  print_list(l);
  avl_assert_valid(l);
  return r;
}

int list_find(list *l, char *item) {
  node *cur;
  int cmp;

  dprintf(DBG_LIST_OPS, "%p: searching for %s\n", l, item);
  cur = l->head;
  while (cur) {
    cmp = strcmp(item, cur->item);
    if (cmp == 0) {
      dprintf(DBG_LIST_OPS, "   (found)\n");
      return 1;
    }
    cur = cur->children[cmp > 0];
    }
  dprintf(DBG_LIST_OPS, "   (not found)\n");
  return 0;
}

void avl_dispose(node *root, int strings) {
  if (root != NULL) {
    avl_dispose(root->children[0], strings);
    avl_dispose(root->children[1], strings);
    if (strings)
      free(root->item);
    free(root);
  }
}

void list_dispose(list *l, int strings) {
  dprintf(DBG_LIST_OPS, "%p: disposing (strings = %d)\n", strings);
  avl_dispose(l->head, strings);
  l->head = NULL;
}

void list_free(list *l, int strings) {
  dprintf(DBG_LIST_OPS, "%p: freeing (strings = %d)\n", strings);
  list_dispose(l, strings);
  free(l);
}

void avl_walk(node *root, void (*walker)(char *item, void *data), void *data) {
  if (root != NULL) {
    avl_walk(root->children[0], walker, data);
    dprintf(DBG_LIST_WALKER, "Walking: %s\n", root->item);
    walker(root->item, data);
    avl_walk(root->children[1], walker, data);
  }
}

void list_walk(list *l, void (*walker)(char *item, void *data), void *data) {
  dprintf(DBG_LIST_OPS, "%p: walker\n");
  avl_walk(l->head, walker, data);
}

void merge_walker(char *item, void *data) {
  list_insert((list *) data, item);
}

void list_merge(list *list1, list *list2) {
  dprintf(DBG_LIST_OPS, "merging %p into %p\n", list2, list1);
  list_walk(list2, merge_walker, (void *) list1);
}

void subtract_walker(char *item, void *data) {
  list_remove((list *) data, item);
}

void list_subtract(list *list1, list *list2) {
  dprintf(DBG_LIST_OPS, "subtracting %p from %p\n", list2, list1);
  list_walk(list2, subtract_walker, (void *) list1);
}

void mask_walker(char *item, void *data) {
  list **data2 = (list **) data;

  if (list_find(data2[0], item)) list_insert(data2[1], item);
}

void list_mask(list *list1, list *list2) {
  list *small, *big, *tmp;
  list *data[2];
  node *old;

  dprintf(DBG_LIST_OPS, "masking %p with %p\n", list1, list2);
  tmp = list_alloc();
  if (GET_DEPTH(list1->head) < GET_DEPTH(list2->head)) {
    small = list1;
    big = list2;
  }
  else {
    small = list2;
    big = list1;
  }
  data[0] = big;
  data[1] = tmp;
  list_walk(small, mask_walker, data);
  list_dispose(list1, 1);
  list1->head = tmp->head;
  free(tmp);
}
