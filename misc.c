/*
 $Id: misc.c,v 1.2 2002/01/05 22:53:14 bruce Exp $

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

/*
 * misc.c provides a mechanism for quitting with an error message, and a version of malloc
 * that will call it if allocation fails.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_REGEX_H
# include <regex.h>
#endif

void die(const char *msg, ...) {
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

void pdie(const char *msg) {
  perror(msg);
  exit(1);
}

void *safe_malloc(size_t size) {
  void *tmp = malloc(size);
  if (!tmp)
    pdie("Allocation failed: ");
  return tmp;
}

char *duplicate(const char *str) {
  char *tmp;

  tmp = (char *) safe_malloc(strlen(str) + 1);
  strcpy(tmp, str);
  return tmp;
}

void *regex_init(const char *regex) {
  regex_t *preg;

  preg = (regex_t *) safe_malloc(sizeof(regex_t));
  if (regcomp(preg, regex, REG_EXTENDED | REG_NOSUB) != 0) {
    free(preg);
    return NULL;
  }
  else return preg;
}

int regex_test(const char *string, void *handle) {
#if HAVE_REGCOMP
  return (regexec((regex_t *) handle, string, 0, NULL, 0) != 0);
#else
# error "jinamp does not yet support calling egrep"
#endif
}

void regex_done(void *handle) {
  if (handle != NULL) {
    regfree((regex_t *) handle);
    free(handle);
  }
}
