/*
 $Id: debug.c,v 1.3 2002/05/16 17:40:35 bruce Exp $

 jinamp: a command line music shuffler
 Copyright (C) 2001, 2002  Bruce Merry.

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
#include <stdarg.h>

#include <debug.h>

#if DEBUG

unsigned int debug_flags = 0;

void dprintf(unsigned int mask, char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  if (debug_flags & mask)
    vfprintf(stderr, fmt, args);
  va_end(args);
}

#else /* DEBUG */

void dprintf(unsigned int mask, char *fmt, ...) {
}

#endif /* DEBUG */
