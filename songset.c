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
#include <stddef.h>

#include <songset.h>
#include <misc.h>
#include <debug.h>

#define GET_DEPTH(node) (((node) ? (node)->depth : -1))
#define TOO_SHALLOW(node, side) (GET_DEPTH((node)->children[(side)]) < (node)->depth - 2)

struct songset *set_alloc(void)
{
    struct songset *tmp = (struct songset *) safe_malloc(sizeof(struct songset));

    dprintf(DBG_SET_OPS, "%p: set created\n", (void *) tmp);
    set_init(tmp);
    return tmp;
}

void set_init(struct songset *l)
{
    dprintf(DBG_SET_OPS, "%p: set initialised\n", (void *) l);
    l->root = NULL;
    l->head = NULL;
}

size_t set_size(const struct songset *l)
{
    size_t tot = 0;
    struct song *i;

    if (!l->head) return 0;
    i = l->head;
    do
    {
        i = i->next;
        tot++;
    } while (i != l->head);
    return tot;
}

int set_empty(const struct songset *l)
{
    return l->head == NULL;
}

static void avl_fix_depth(struct song *n)
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
static void avl_assert_order(const struct song *root, const char **first, const char **last)
{
    const char *tmp;

    *first = *last = NULL;
    if (root->children[0])
    {
        avl_assert_order(root->children[0], first, &tmp);
        if (strcmp(root->name, tmp) <= 0)
        {
            if (strcmp(root->name, tmp) == 0)
                die("Duplicate key: %s\n", root->name);
            else
                die("Out of order: %s before %s\n", tmp, root->name);
        }
    }
    if (root->children[1])
    {
        avl_assert_order(root->children[1], &tmp, last);
        if (strcmp(root->name, tmp) >= 0)
        {
            if (strcmp(root->name, tmp) == 0)
                die("Duplicate key: %s\n", root->name);
            else
                die("Out of order: %s before %s\n", root->name, tmp);
        }
    }
    if (!(*first)) *first = root->name;
    if (!(*last)) *last = root->name;
}

static void avl_assert_depth(const struct song *root)
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
        die("assert_depth failed on node %s: wrong depth count", root->name);
    for (i = 0; i < 2; i++)
        if (TOO_SHALLOW(root, i))
            die("assert_depth failed on node %s: not balanced", root->name);
}

static void avl_print_tree(const struct song *root, int offset)
{
    if (root->children[0]) avl_print_tree(root->children[0], offset + 3);
    dprintf(DBG_SET_SHOW, "%*c%s\n", offset, ' ', root->name);
    if (root->children[1]) avl_print_tree(root->children[1], offset + 3);
}

/* checks that the AVL tree is valid */
static void avl_assert_valid(const struct songset *l)
{
    const char *first, *last;                       /* dummy */
    dprintf(DBG_SET_OPS, "%p: asserting AVL tree\n", (void *) l);
    if (l->root)
    {
        avl_assert_order(l->root, &first, &last);
        avl_assert_depth(l->root);
    }
}

static void print_set(const struct songset *l)
{
    dprintf(DBG_SET_SHOW, "Printing set %p\n", (void *) l);
    if (debug_flags && DBG_SET_SHOW && l->root)
        avl_print_tree(l->root, 2);
}
#else /* DEBUG */
#define avl_assert_valid(root)
#define print_set(l)
#endif /* DEBUG */

/* child is the side that is too deep */
static void avl_single_rotate(struct song **root, int child)
{
    struct song *n, *tmp;

    n = *root;
    tmp = n->children[child];
    n->children[child] = tmp->children[!child];
    tmp->children[!child] = n;
    *root = tmp;
    avl_fix_depth(n);
    avl_fix_depth(tmp);
}

/* child is the side that is too deep */
static void avl_double_rotate(struct song **root, int child)
{
    struct song *n, *tmp;

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

static struct song *clone_node(const struct song *node)
{
    struct song *clone;

    clone = safe_malloc(sizeof(struct song));
    clone->children[0] = clone->children[1] = NULL;
    clone->depth = 0;
    clone->prev = clone->next = NULL;

    clone->name = duplicate(node->name);
    clone->repeat = node->repeat;
    return clone;
}

/* Inserts an item into a subtree, balancing if necessary. A return of 2 indicates
 * successful insertion and balancing was done, 1 indicates insertion w/o balancing
 * and 0 indicates that the key was a duplicate.
 */
static int avl_insert(struct song **root,
                      const struct song *node,
                      struct song **clone)
{
    int r, sub;
    int old_depth;
    struct song *n;
    int child;

    n = *root;
    r = strcmp(node->name, n->name);
    if (r == 0) /* duplicate */
        return 0;
    child = r > 0;
    if (!n->children[child])
    {
        n->children[child] = *clone = clone_node(node);
        if (n->depth == 0) n->depth = 1;
        return 1;
    }
    sub = avl_insert(&n->children[child], node, clone);
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

/* returns the new node for successful insert, NULL for duplicate
 * The passed struct is copied and may be freed afterwards, regardless
 * of whether it was inserted.
 */
struct song *set_insert(struct songset *l, const struct song *song)
{
    struct song *clone = NULL;
    int r;

    dprintf(DBG_SET_OPS, "%p: inserting %s\n", (void *) l, song->name);

    if (l->root == NULL)
    {
        l->root = l->head = clone = clone_node(song);
        l->head->prev = l->head;
        l->head->next = l->head;
    }
    else
    {
        r = avl_insert(&l->root, song, &clone);
        if (r)
        {
            clone->prev = l->head->prev;
            clone->next = l->head;
            l->head->prev->next = clone;
            l->head->prev = clone;
        }
        else
            clone = NULL;
    }
    print_set(l);
    avl_assert_valid(l);
    return clone;
}

/* Returns:
 * 0: removed, no further balancing required
 * 1: removed, possible balancing required
 *
 * The compare parameter may be set to -1 or 1 if the comparison is
 * already known. 0 means unknown.
 *
 * The song is not actually freed, just unlinked from the tree. The
 * caller unlinks from the ring.
 */
static int avl_remove(struct song **root, struct song *song, int compare)
{
    struct song *r;
    int status;
    int i;

    r = *root;
    if (r == song)
    {
        if (song->children[0] || song->children[1])
        {
            int depth;
            struct song **child, *c, *tmp;

            i = song->children[0] ? 0 : 1;
            child = &song->children[i];
            while ((*child)->children[!i])
                child = &(*child)->children[!i];
            c = *child;
            if (child == &song->children[i]) child = &c->children[i];

            /* Swap the tree parts of the two structs, as well as the
             * incoming pointers. The careful ordering, plus the if
             * statement above, ensure that the case where child is
             * a direct child of song is handled.
             */
            depth = song->depth; song->depth = c->depth; c->depth = depth;
            tmp = song->children[0]; song->children[0] = c->children[0]; c->children[0] = tmp;
            tmp = song->children[1]; song->children[1] = c->children[1]; c->children[1] = tmp;
            *root = c;
            *child = r;
            r = c;

            status = avl_remove(&r->children[i], song, i ? -1 : 1);
        }
        else
        {
            *root = NULL;
            return 1;
        }
    }
    else
    {
        int c;

        c = compare ? compare : strcmp(song->name, r->name);
        i = c > 0;
        status = avl_remove(&r->children[i], song, compare);
    }

    if (status)                        /* may need to balance */
    {
        int old_depth;

        old_depth = r->depth;
        if (TOO_SHALLOW(r, i))
        {
            if (GET_DEPTH(r->children[!i]->children[i]) > GET_DEPTH(r->children[i])
                && GET_DEPTH(r->children[!i]->children[!i]) == GET_DEPTH(r->children[i]))
                avl_double_rotate(root, !i);
            else
                avl_single_rotate(root, !i);
        }
        else
            avl_fix_depth(r);
        return old_depth != (*root)->depth;
    }
    else
        return 0;
}

void set_erase(struct songset *set, struct song *song)
{
    dprintf(DBG_SET_OPS, "%p: erasing %s\n", (void *) set, song->name);

    avl_remove(&set->root, song, 0);
    if (set->root == NULL) set->head = NULL;
    else
    {
        song->prev->next = song->next;
        song->next->prev = song->prev;
        if (song == set->head) set->head = set->head->next;
    }
    free(song->name);
    free(song);
    print_set(set);
    avl_assert_valid(set);
}

/* returns true if key found and removed, false if not found */
int set_remove(struct songset *set, const char *key)
{
    struct song *song;

    dprintf(DBG_SET_OPS, "%p: removing %s\n", (void *) set, key);

    song = set_get(set, key);
    if (!song) return 0;
    else
    {
        set_erase(set, song);
        return 1;
    }
}

int set_find(const struct songset *set, const char *key)
{
    struct song *cur;
    int cmp;

    dprintf(DBG_SET_OPS, "%p: searching for %s\n", (void *) set, key);
    cur = set->root;
    while (cur)
    {
        cmp = strcmp(key, cur->name);
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

/* Like set_find, but returns the associated struct, or NULL */
struct song *set_get(struct songset *set, const char *key)
{
    struct song *cur;
    int cmp;

    dprintf(DBG_SET_OPS, "%p: searching for %s\n", (void *) set, key);
    cur = set->root;
    while (cur)
    {
        cmp = strcmp(key, cur->name);
        if (cmp == 0)
        {
            dprintf(DBG_SET_OPS, "   (found)\n");
            return cur;
        }
        cur = cur->children[cmp > 0];
    }
    dprintf(DBG_SET_OPS, "   (not found)\n");
    return NULL;
}

void set_dispose(struct songset *set)
{
    struct song *i, *next;

    dprintf(DBG_SET_OPS, "%p: disposing\n", (void *) set);

    if (set->head) set->head->prev->next = NULL;
    i = set->head;
    while (i)
    {
        next = i->next;
        free(i->name);
        free(i);
        i = next;
    }
    set->head = NULL;
    set->root = NULL;
}

void set_free(struct songset *set)
{
    dprintf(DBG_SET_OPS, "%p: freeing\n", (void *) set);
    set_dispose(set);
    free(set);
}

void set_merge(struct songset *set1, const struct songset *set2)
{
    struct song *i;
    dprintf(DBG_SET_OPS, "merging %p into %p\n", (const void *) set2, (void *) set1);

    if (!set2->head) return;
    i = set2->head;
    do
    {
        set_insert(set1, i);
        i = i->next;
    } while (i != set2->head);
}

void set_subtract(struct songset *set1, const struct songset *set2)
{
    struct song *i;

    dprintf(DBG_SET_OPS, "subtracting %p from %p\n", (const void *) set2, (void *) set1);
    if (!set2->head) return;
    i = set2->head;
    do
    {
        set_remove(set1, i->name);
        i = i->next;
    } while (i != set2->head);
}

void set_mask(struct songset *set1, const struct songset *set2)
{
    struct song *i, *next;
    int more;

    dprintf(DBG_SET_OPS, "masking %p with %p\n", (void *) set1, (const void *) set2);

    if (!set1) return;
    i = set1->head;
    do
    {
        next = i->next;
        more = (next != set1->head);
        if (!set_find(set2, i->name))
            set_erase(set1, i);
        i = next;
    } while (more);
}

/* Removes song from its ring, returning the following item */
static struct song *set_sort_unlink(struct song *song)
{
    song->prev->next = song->next;
    song->next->prev = song->prev;
    return (song == song->next) ? NULL : song->next;
}

/* Places song immediately before ring, returning the new ring */
static struct song *set_sort_append(struct song *ring, struct song *song)
{
    if (!ring)
        return song->prev = song->next = song;
    else
    {
        song->next = ring;
        song->prev = ring->prev;
        ring->prev->next = song;
        ring->prev = song;
        return ring;
    }
}

/* Does a recursive mergesort on the ring starting at 'ring', returning
 * the new first element.
 *
 * Note that while returning a random value for key comparison is
 * normally a bad thing to do to a sorting algorithm (because it
 * may introduce inconsistencies), mergesort is nice because it never
 * performs comparisons that could lead to inconsistency.
 */
static struct song *set_sort_merge(struct song *ring, enum songset_key key)
{
    struct song *pivotl, *pivotr, *search, *last, **choice;
    size_t size_left = 0, size_right = 0;
    int compare = 0;

    if (ring->next == ring) return ring; /* 1 element */
    /* Identify the middle by moving search at half the speed of search
     * until it reaches the end.
     */
    last = ring->prev;
    search = ring;
    pivotl = ring;
    while (search != last && search->next != last)
    {
        pivotl = pivotl->next;
        search = search->next->next;
        size_left++;
        size_right++;
    }
    size_left++;
    if (search != last) size_right++;
    pivotr = pivotl->next;

    /* Split into two separate rings to be recursed upon */
    pivotl->next = ring;
    ring->prev = pivotl;
    last->next = pivotr;
    pivotr->prev = last;

    pivotl = set_sort_merge(ring, key);
    pivotr = set_sort_merge(pivotr, key);

    /* Merge */
    ring = NULL;
    while (pivotl || pivotr)
    {
        if (pivotr == NULL) compare = 1;
        else if (pivotl == NULL) compare = 0;
        else switch(key)
        {
        case KEY_ORIGINAL:
            compare = 0; break;
        case KEY_ALPHABETICAL:
            compare = strcmp(pivotl->name, pivotr->name) < 0; break;
        case KEY_RANDOM:
            compare = rand() % (size_left + size_right) < size_left; break;
        }
        choice = compare ? &pivotl : &pivotr;
        if (compare) size_left--; else size_right--;
        search = *choice;
        *choice = set_sort_unlink(*choice);
        ring = set_sort_append(ring, search);
    }

    return ring;
}

void set_sort(struct songset *set, enum songset_key key)
{
    if (set->head && key != KEY_ORIGINAL)
        set->head = set_sort_merge(set->head, key);
}
