/*
 $Id: control.c,v 1.2 2002/11/16 21:54:58 bruce Exp $

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

#include <control.h>
#if USING_JINAMP_CTL

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <misc.h>

const char socksuffix[] = "/.jinamp-socket";
char *cleanup_name = NULL; /* FIXME: make this is a list */

void get_sock_name(char *name, int size) {
  char *home;

  home = getenv("HOME");
  if (!home) home = "/";
  if (strlen(home) + strlen(socksuffix) + 1 > size)
    die("home directory name is too long for UNIX domain socket address");
  sprintf(name, "%s%s", home, socksuffix);
}

void cleanup() {
  if (cleanup_name) {
#if DEBUG
    printf("unlinking %s\n", cleanup_name);
#endif
    unlink(cleanup_name);
    free(cleanup_name);
  }
}

/* returns the socket ID on success, -1 on address in use,
   dies on unexpected failure. */
int get_control_socket(int server) {
  struct sockaddr_un addr;
  int sock;

  addr.sun_family = AF_UNIX;
  get_sock_name(addr.sun_path, sizeof(addr.sun_path));
  sock = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (sock == -1) pdie("socket");
  if (server) {
    unlink(addr.sun_path);
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
      return -1;
    cleanup_name = duplicate(addr.sun_path);
    /* FIXME: having more than one */
    if (atexit(cleanup) != 0) die("atexit failed");
  }
  else {
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) pdie("connect");
  }
#if DEBUG
  printf("Socket: %d (%s)\n", sock, addr.sun_path);
#endif
  return sock;
}

void close_control_socket(int sock) {
  char sockname[UNIX_PATH_MAX];

  close(sock);
  get_sock_name(sockname, sizeof(sockname));
}

/* Sends the given packet to the socket, which must already have been bound
 * and connected to the server
 */
void send_control_packet(int socket, const command_t *command, size_t command_len) {
  if (send(socket, command, command_len, 0) == -1)
    pdie("send");
}

/* returns the control packet if there is one, or NULL if the
 * queue is empty. Caller must free the memory. */
command_t *receive_control_packet(int socket) {
  int buffer[1];
  int length;
  command_t *answer;

  length = recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT | MSG_PEEK | MSG_TRUNC);
  if (length == -1) {
    if (errno == EAGAIN) return NULL;
    else pdie("recv");
  }
  answer = (command_t *) safe_malloc(length);
  length = recv(socket, answer, length, MSG_DONTWAIT);
  if (length == -1) pdie("recv");
  return answer;
}

#endif /* USING_JINAMP_CTL */
