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

/*! \file songring.h
 * \brief song ring management
 * This file declares the song ring functions implemented in songring.c.
 * A song ring is a doubly linked ring of song-specific data.
 */

#ifndef JINAMP_SONGRING_H
#define JINAMP_SONGRING_H

#include <stddef.h>

typedef struct song
{
    const char *name;
    size_t order;    /* Number of associated command line argument */
    int once;        /* True if loaded through jinamp-ctl queue */
    struct song *next;
    struct song *prev;
} song;

/* Most functions return a new handle to the ring. */

song *ring_append(song *ring, song *s);
song *ring_remove(song *ring, song *s);
void ring_free(song *ring);
song *ring_sort(song *ring); /* Sort by \c order field */

#endif /* JINAMP_SONGRING_H */
