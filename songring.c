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
#include <stdlib.h>
#include <songring.h>

song *ring_append(song *ring, song *s)
{
    if (!ring)
    {
        s->prev = s->next = s;
        return s;
    }
    else
    {
        s->next = ring;
        s->prev = ring->prev;
        ring->prev->next = s;
        ring->prev = s;
        return ring;
    }
}

static song *ring_unlink(song *ring, song *s)
{
    s->prev->next = s->next;
    s->next->prev = s->prev;
    if (ring == s)
    {
        ring = ring->next;
        if (ring == s) ring = NULL;
    }
    return ring;
}

song *ring_remove(song *ring, song *s)
{
    song *ans;

    ans = ring_unlink(ring, s);
    free(s);
    return ans;
}

void ring_free(song *ring)
{
    while (ring) ring = ring_remove(ring, ring);
}

/* We can't just use qsort, because it's not an array... we also don't want
 * to change the pointers for anything, because they are stored in the song
 * set. We settle for a mergesort.
 * Returns the point to the new first element.
 */
song *ring_sort(song *ring)
{
    song *pivotl, *pivotr, *search, *last;

    if (!ring || ring->next == ring) return ring; /* 0 or 1 element */
    /* Identify the middle by moving search at half the speed of pivot
     * until it reaches the end.
     */
    last = ring->prev;
    search = ring;
    pivotl = ring;
    while (search != last && search->next != last)
    {
        pivotl = pivotl->next;
        search = search->next->next;
    }
    pivotr = pivotl->next;

    /* Split into two separate rings to be recursed upon */
    pivotl->next = ring;
    ring->prev = pivotl;
    last->next = pivotr;
    pivotr->prev = last;

    pivotl = ring_sort(ring);
    pivotr = ring_sort(pivotr);

    /* Merge */
    ring = NULL;
    while (pivotl || pivotr)
    {
        if (pivotr == NULL) search = pivotl;
        else if (pivotl == NULL) search = pivotr;
        else search = (pivotl->order <= pivotr->order) ? pivotl : pivotr;
        if (search == pivotl) pivotl = ring_unlink(pivotl, search);
        else pivotr = ring_unlink(pivotr, search);
        ring = ring_append(ring, search);
    }

    return ring;
}
