/*
 $Id: load.c,v 1.3 2002/05/16 17:40:35 bruce Exp $
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
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include <misc.h>
#include <load.h>
#include <list.h>
#include <debug.h>

void read_list(char *listname, list *names, list *done,
               void *playlist_handle, void *exclude_handle) {
  FILE *fd;
  char *buffer;
  size_t size;
  char *ptr;
  size_t len;

  if (!(fd = fopen(listname, "r"))) pdie("read_list");
  
  size = 1024;
  buffer = (char *) safe_malloc(sizeof(char) * size);
  
  while (!feof(fd)) {
    ptr = buffer;
    buffer[0] = '\0';
    do {
      if (!fgets(ptr, size, fd)) break;
      len = strlen(ptr);
      if (len == size - 1 && ptr[len - 1] != '\n') {
	if (!realloc(buffer, sizeof(char) * size * 2)) pdie("read_list");
	ptr += len;
	size *= 2;
      }
      else break;
    } while (1);
    len = strlen(buffer);
    if (!len) break;
    while (len >= 1 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n'))
      buffer[--len] = '\0';

    read_object(buffer, names, done, playlist_handle, exclude_handle);
  }
  fclose(fd);
  free(buffer);
}

void read_directory(char *dirname, list *names, list *done,
                    void *playlist_handle, void *exclude_handle) {
  DIR *directory;
  struct dirent *entry;
  char *buffer;
  int bufsize;

  buffer = NULL;
  bufsize = 0;

  if ((directory = opendir(dirname)) == NULL)
    pdie("Failed to open directory");
  while ((entry = readdir(directory)) != NULL)
    if (entry->d_name[0] != '.') {
      if (buffer == NULL) {
        bufsize = strlen(dirname) + strlen(entry->d_name) + 2;
        buffer = (char *) safe_malloc(bufsize);
      }
      else if (strlen(dirname) + strlen(entry->d_name) + 2 > bufsize) {
        free(buffer);
        bufsize = strlen(dirname) + strlen(entry->d_name) + 2;
        buffer = (char *) safe_malloc(bufsize);
      }
      strcpy(buffer, dirname);
      if (dirname[0] != '\0' && dirname[strlen(dirname) - 1] != '/')
        strcat(buffer, "/");
      strcat(buffer, entry->d_name);
      read_object(buffer, names, done, playlist_handle, exclude_handle);
    }
  if (buffer != NULL) free(buffer);
  closedir(directory);
}

void read_object(char *file, list *names, list *done,
                void *playlist_handle, void *exclude_handle) {
  struct stat buf;
  int r;
  char *canon;
  int path_max;

  if (list_find(done, file)) return; /* already processed */

  /* check that there is something there */
  r = stat(file, &buf);
  if (r == -1) {
    printf("Warning: cannot access %s, skipping\n", file);
    /* avoid further errors */
    if (list_find(done, file)) return;
    return;
  }
  if (access(file, R_OK)) {
    printf("Warning: cannot read %s, skipping\n", file);
    /* avoid further errors */
    if (list_find(done, file)) return;
    return;
  }

  /* canonicalise the pathname */
#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
# if HAVE_PATHCONF
  path_max = pathconf(file, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
# else
  path_max = 4096;
# endif
#endif
#if HAVE_REALPATH
  canon = (char *) safe_malloc(path_max);
  realpath(file, canon);
#else
  canon = (char *) safe_malloc(strlen(file) + 1);
  strcpy(canon, file);
#endif

  if (exclude_handle == NULL || regex_test(canon, exclude_handle) != 0) {
    if (S_ISDIR(buf.st_mode)) {
      list_insert(done, canon);
      dprintf(DBG_LOAD_DONE, "Added %s to done list\n", canon);
      read_directory(canon, names, done, playlist_handle, exclude_handle);
    }
    else if (S_ISREG(buf.st_mode)) {
      if (playlist_handle && regex_test(canon, playlist_handle) == 0) {
        list_insert(done, canon);
        dprintf(DBG_LOAD_DONE, "Added %s to done list\n", canon);
        read_list(canon, names, done, playlist_handle, exclude_handle);
      }
      else {
        list_insert(names, canon);
        dprintf(DBG_LOAD_SHOW, "%p: Added %s\n", names, canon);
      }
    }
    else
      printf("Warning: %s is not a regular file or directory, skipping\n", canon);
  }
  else
    dprintf(DBG_LOAD_REGEX, "Excluded %s\n", canon);
  free(canon);
}
