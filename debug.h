/*
 $Id: debug.h,v 1.8 2005/04/25 15:16:31 bruce Exp $

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

#ifndef JINAMP_DEBUG_H
#define JINAMP_DEBUG_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* The dprintf function is used for debug output elsewhere. If debug_flags & mask is
 * non-zero, then the message is printed printf style to stderr. The debug_flags is
 * intended to be set inside a debugger.
 */

#define DBG_LIST_OPS          0x00000001      /* shows each AVL op */
#define DBG_LIST_SHOW         0x00000002      /* shows tree after each tree op */
#define DBG_LIST_WALKER       0x00000004      /* shows each item as it is walked */

#define DBG_CONFIG_COUNT      0x00000010      /* shows how many config items found */
#define DBG_CONFIG_INFO       0x00000020

#define DBG_LOAD_SHOW         0x00000100
#define DBG_LOAD_REGEX        0x00000200
#define DBG_LOAD_DONE         0x00000400

#define DBG_CONTROL_ERRORS    0x00001000
#define DBG_CONTROL_DATA      0x00002000

#define DBG_MISC              0x80000000

#if DEBUG

extern unsigned int debug_flags;

#endif /* DEBUG */

#ifdef __GNUC__
# define DPRINTF_ATTRIBUTES __attribute__ ((format(printf, 2, 3)))
#else
# define DPRINTF_ATTRIBUTES
#endif
void dprintf(unsigned int mask, char *fmt, ...) DPRINTF_ATTRIBUTES;

#endif /* JINAMP_DEBUG_H */
