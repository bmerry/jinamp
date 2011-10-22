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

/*
 * misc.c provides a mechanism for quitting with an error message, and a version of malloc
 * that will call it if allocation fails, plus some other arb functions.
 */

#ifndef JINAMP_MISC_H
#define JINAMP_MISC_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdarg.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

/* Prints msg to stderr (fprintf style) and exits with status 1 */
void die(const char *msg, ...);

/* Similar to die but calls perror with msg. */
void pdie(const char *msg);

/* Wrapper around malloc that prints an error and exits on failure */
void *safe_malloc(size_t size);

/* Duplicates the given string and returns the pointer */
char *duplicate(const char *str);

/* Like strncpy, but always leaves space for and inserts a NULL terminator */
char *my_strncpy(char *dst, const char *src, size_t n);

/* Wrappers around regcomp/regexec/regfree that will simply call egrep
 * on systems that lack these (they are specified by POSIX.2).
 * Only whole-word extended regex's are supported.
 */

/* returns a handle to be used to regex_test and regex_done, or NULL if
 * the regex is malformed.
 */
void *regex_init(const char *regex);

/* Returns 0 for match, 1 for non-match, 2 for other errors (e.g. egrep
 * not found).
 */
int regex_test(char *string, void *handle);

/* frees any associated memory */
void regex_done(void *handle);

#endif /* JINAMP_MISC_H */
