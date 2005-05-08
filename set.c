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

#include <set.h>
#include <misc.h>
#include <debug.h>

#define GET_DEPTH(node) (((node) ? (node)->depth : -1))
#define TOO_SHALLOW(node, side) (GET_DEPTH((node)->children[(side)]) < (node)->depth - 2)

set *set_alloc(int owns_values)
{
    set *tmp = (set *) safe_malloc(sizeof(set));

    dprintf(DBG_SET_OPS, "%p: set created\n", (void *) tmp);
    set_init(tmp, owns_values);
    return tmp;
}

void set_init(set *l, int owns_values)
{
    dprintf(DBG_SET_OPS, "%p: set initialised\n", (void *) l);
    l->head = NULL;
    l->owns_values = owns_values;
}

static void count_walker(const char *key, void **value, void *data)
{
    (*((size_t *) data))++;
}

size_t set_count(const set *l)
{
    size_t tot = 0;
    set_walk((set *) l, count_walker, (void *) &tot);
    dprintf(DBG_SET_OPS, "%p: counted: %ld elements\n", (const void *) l, (long) tot);
    return tot;
}

static void avl_fix_depth(node *n)
{
    int i;

    n->depth = 0;
    for (i = 0; i < 2; i++)
        if (n->children[i] && n->children[i]->depth >= n->depth)
            n->depth = n->children[i]->depth + 1;
}

#if DEBUG

/* checks that the tree is properly sorted w/o duplication, and returns the
 * first and last key. root must exist.
 */
static void avl_assert_order(const node *root, const char **first, const char **last)
{
    const char *tmp;

    *first = *last = NULL;
    if (root->children[0])
    {
        avl_assert_order(root->children[0], first, &tmp);
        if (strcmp(root->key, tmp) <= 0)
        {
            if (strcmp(root->key, tmp) == 0)
                die("Duplicate key: %s\n", root->key);
            else
                die("Out of order: %s before %s\n", tmp, root->key);
        }
    }
    if (root->children[1])
    {
        avl_assert_order(root->children[1], &tmp, last);
        if (strcmp(root->key, tmp) >= 0)
        {
            if (strcmp(root->key, tmp) == 0)
                die("Duplicate key: %s\n", root->key);
            else
                die("Out of order: %s before %s\n", root->key, tmp);
        }
    }
    if (!(*first)) *first = root->key;
    if (!(*last)) *last = root->key;
}

static void avl_assert_depth(const node *root)
{
    int i;
    int depth;

    depth = 0;
    for (i = 0; i < 2; i++)
        if (root->children[i])
        {
            avl_assert_depth(root->children[i]);
            if (root->children[i]->depth >= depth)
                depth = root->children[i]->depth + 1;
        }
    if (root->depth != depth)
        die("assert_depth failed on node %s: wrong depth count", root->key);
    for (i = 0; i < 2; i++)
        if (TOO_SHALLOW(root, i))
            die("assert_depth failed on node %s: not balanced", root->key);
}

static void avl_print_tree(const node *root, int offset)
{
    if (root->children[0]) avl_print_tree(root->children[0], offset + 3);
    dprintf(DBG_SET_SHOW, "%*c%s\n", offset, ' ', root->key);
    if (root->children[1]) avl_print_tree(root->children[1], offset + 3);
}

/* checks that the AVL tree is valid */
static void avl_assert_valid(const set *l)
{
    const char *first, *last;                       /* dummy */
    dprintf(DBG_SET_OPS, "%p: asserting AVL tree\n", (void *) l);
    avl_assert_order(l->head, &first, &last);
    avl_assert_depth(l->head);
}

static void print_set(const set *l)
{
    dprintf(DBG_SET_SHOW, "Printing set %p\n", (void *) l);
    if (debug_flags && DBG_SET_SHOW && l->head)
        avl_print_tree(l->head, 2);
}
#else /* DEBUG */
#define avl_assert_valid(root)
#define print_set(l)
#endif /* DEBUG */

/* child is the side that is too deep */
static void avl_single_rotate(node **root, int child)
{
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
static void avl_double_rotate(node **root, int child)
{
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
 * and 0 indicates that the key was a duplicate (but value is still set).
 */
static int avl_insert(node **root, const char *key, void *value)
{
    int r, sub;
    int old_depth;
    node *n;
    int child;

    n = *root;
    r = strcmp(key, n->key);
    if (r == 0) /* duplicate */
    {
        n->value = value;
        return 0;
    }
    child = r > 0;
    if (!n->children[child])
    {
        n->children[child] = (node *) safe_malloc(sizeof(node));
        n->children[child]->children[0] = n->children[child]->children[1] = NULL;
        n->children[child]->depth = 0;
        n->children[child]->key = duplicate(key);
        n->children[child]->value = value;
        if (n->depth == 0) n->depth = 1;
        return 1;
    }
    sub = avl_insert(&n->children[child], key, value);
    if (sub == 2) return 2;              /* tree has already been balanced */
    else if (sub == 1)
    {
        old_depth = n->depth;
        avl_fix_depth(n);
        if (TOO_SHALLOW(n, !child))
        {
            if (GET_DEPTH(n->children[child]->children[child]) == n->children[child]->depth - 1)
                avl_single_rotate(root, child);
            else
                avl_double_rotate(root, child);
        }
        return (old_depth == n->depth) ? 2 : 1;
    }
    return 0;
}

/* returns true for successful insert, false for duplicate (but value is always set) */
int set_insert(set *l, const char *key, void *value)
{
    node *tmp;
    int r;

    dprintf(DBG_SET_OPS, "%p: inserting %s\n", (void *) l, key);
    if (l->head == NULL)
    {
        tmp = (node *) safe_malloc(sizeof(node));
        tmp->children[0] = NULL;
        tmp->children[1] = NULL;
        tmp->key = duplicate(key);
        tmp->value = value;
        tmp->depth = 0;
        l->head = tmp;
        r = 1;
    }
    else
        r = avl_insert(&l->head, key, value);
    print_set(l);
    avl_assert_valid(l);
    return r;
}

/* returns:
 * 0: not found
 * 1: removed
 * 2: removed, no further balancing required
 */
static int avl_remove(node **root, const char *key,
                      int free_keys, int free_values)
{
    int cmp;
    int sub;                   /* return code from subcalls */
    int i;
    int old_depth;
    node *tmp, *n;

    n = *root;
    if (!n) return 0;
    cmp = strcmp(key, n->key);
    sub = -1;
    if (cmp == 0)                        /* found it, now make it a leaf */
    {
        if (free_keys)
            free(n->key);
        if (free_values && n->value)
            free(n->value);
        for (i = 0; i < 2; i++)
            if (n->children[i])
            {
                tmp = n->children[i];
                while (tmp->children[!i])
                    tmp = tmp->children[!i];
                n->key = tmp->key;
                n->value = tmp->value;
                sub = avl_remove(&n->children[i], tmp->key, 0, 0);
                if (!sub) die("avl_remove subcall failed???");
                break;
            }
        if (sub == -1)
        {
            *root = NULL;
            return 1;
        }
    }
    else
    {
        i = cmp > 0;
        sub = avl_remove(&n->children[i], key, free_keys, free_values);
    }
    if (sub == 1)                        /* may need to balance */
    {
        old_depth = n->depth;
        if (TOO_SHALLOW(n, i))
        {
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

/* returns true if key found and removed, false if not found */
int set_remove(set *l, const char *key)
{
    int r;

    dprintf(DBG_SET_OPS, "%p: removing %s\n", (void *) l, key);
    if (l->head == NULL)
        r = 0;
    else
        r = (avl_remove(&l->head, key, 1, l->owns_values) != 0);
    print_set(l);
    avl_assert_valid(l);
    return r;
}

int set_find(const set *l, const char *key)
{
    node *cur;
    int cmp;

    dprintf(DBG_SET_OPS, "%p: searching for %s\n", (void *) l, key);
    cur = l->head;
    while (cur)
    {
        cmp = strcmp(key, cur->key);
        if (cmp == 0)
        {
            dprintf(DBG_SET_OPS, "   (found)\n");
            return 1;
        }
        cur = cur->children[cmp > 0];
    }
    dprintf(DBG_SET_OPS, "   (not found)\n");
    return 0;
}

/* Like set_find, but returns the associated value, or NULL if not found.
 * Note that NULL does not necessarily mean not found, because the associated
 * value may be NULL.
 */
void *set_get(const set *l, const char *key)
{
    node *cur;
    int cmp;

    dprintf(DBG_SET_OPS, "%p: searching for %s\n", (void *) l, key);
    cur = l->head;
    while (cur)
    {
        cmp = strcmp(key, cur->key);
        if (cmp == 0)
        {
            dprintf(DBG_SET_OPS, "   (found)\n");
            return cur->value;
        }
        cur = cur->children[cmp > 0];
    }
    dprintf(DBG_SET_OPS, "   (not found)\n");
    return NULL;
}

static void avl_dispose(node *root, int free_keys, int free_values)
{
    if (root != NULL)
    {
        avl_dispose(root->children[0], free_keys, free_values);
        avl_dispose(root->children[1], free_keys, free_values);
        if (free_keys) free(root->key);
        if (free_values && root->value) free(root->value);
        free(root);
    }
}

void set_dispose(set *l)
{
    dprintf(DBG_SET_OPS, "%p: disposing\n", (void *) l);
    avl_dispose(l->head, 1, l->owns_values);
    l->head = NULL;
}

void set_free(set *l)
{
    dprintf(DBG_SET_OPS, "%p: freeing\n", (void *) l);
    set_dispose(l);
    free(l);
}

static void avl_walk(node *root, void (*walker)(const char *key, void **value, void *data), void *data)
{
    if (root != NULL)
    {
        avl_walk(root->children[0], walker, data);
        dprintf(DBG_SET_WALKER, "Walking: %s\n", root->key);
        walker(root->key, &root->value, data);
        avl_walk(root->children[1], walker, data);
    }
}

void set_walk(set *l, void (*walker)(const char *key, void **value, void *data), void *data)
{
    dprintf(DBG_SET_OPS, "%p: walker\n", (const void *) l);
    avl_walk(l->head, walker, data);
}

static void merge_walker(const char *key, void **value, void *data)
{
    set_insert((set *) data, key, *value);
}

void set_merge(set *set1, const set *set2)
{
    dprintf(DBG_SET_OPS, "merging %p into %p\n", (const void *) set2, (void *) set1);
    set_walk((set *) set2, merge_walker, (void *) set1);
}

static void subtract_walker(const char *key, void **value, void *data)
{
    set_remove((set *) data, key);
}

void set_subtract(set *set1, const set *set2)
{
    dprintf(DBG_SET_OPS, "subtracting %p from %p\n", (const void *) set2, (void *) set1);
    set_walk((set *) set2, subtract_walker, (void *) set1);
}

typedef struct
{
    const set *dict;
    set *output;
} mask_data;

static void mask_walker(const char *key, void **value, void *data)
{
    const mask_data *d = (const mask_data *) data;
    if (set_find(d->dict, key)) set_insert(d->output, key, *value);
}

static void mask_clear_value_walker(const char *key, void **value, void *data)
{
    const mask_data *d = (const mask_data *) data;
    if (*value && !set_find(d->dict, key)) free(*value);
}

void set_mask(set *set1, const set *set2)
{
    mask_data data;

    dprintf(DBG_SET_OPS, "masking %p with %p\n", (void *) set1, (const void *) set2);
    data.dict = set2;
    data.output = set_alloc(0);
    set_walk(set1, mask_walker, &data);
    /* If necessary, free the values that will no longer be present */
    if (set1->owns_values)
    {
        set_walk(set1, mask_clear_value_walker, &data);
        data.output->owns_values = 1;
        set1->owns_values = 0;
    }
    set_dispose(set1);
    set1->head = data.output->head;
    set1->owns_values = data.output->owns_values;
    free(data.output);
}
