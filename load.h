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

#ifndef JINAMP_LOAD_H
#define JINAMP_LOAD_H

#include <songset.h>

/* Adds the named file, directory or list recursively.
 * The `done' parameter is a list of things to ignore (intended for
 * directories and lists, which aren't already in names)
 * `value' is set as the value on files loaded into names
 */
void read_object(const char *file, struct songset *names, struct songset *done,
                 void *playlist_handle, void *exclude_handle);

#endif /* JINAMP_LOAD_H */
