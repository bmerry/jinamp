/*
 $Id: control.h,v 1.6 2002/12/15 01:00:08 bruce Exp $

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

#ifndef JINAMP_CONTROL_H
#define JINAMP_CONTROL_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_SYS_IPC_H
# define USING_JINAMP_CTL 1
#endif

#if USING_JINAMP_CTL
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

typedef enum {
  COMMAND_WAKE,
  COMMAND_NEXT,
  COMMAND_LAST,
  COMMAND_PAUSE,
  COMMAND_CONTINUE,
  COMMAND_STOP,
  COMMAND_REPLACE,
  COMMAND_QUERY,
  REPLY_QUERY
} command_type_t;

/* any payload appears in an extended form of the data structure, in the same
 * way sockaddr_t is extended for various address formats.
 */
typedef struct {
  command_type_t command;
} command_t;

typedef struct {
  command_type_t command;
  int argc;
  char argv[4000]; /* leaves lots of room in case padding happens */
} command_list_t;

typedef struct {
  command_type_t command;
  char value[4000];
} command_string_t;

/* returns the socket ID on success, -1 on failure. */
int get_control_socket(int server);

void close_control_socket(int sock, int server);

/* Sends the given packet to the socket, which must already have been bound
 * and connected to the server
 */
int send_control_packet(int socket, const command_t *command, size_t command_len, int wait, int toserver);

/* returns the control packet if there is one, or NULL if the queue
 * is empty. Caller must free the memory. len is the maximum length to
 * receive; beyond this truncation occurs. Returns the actual size on
 * success or -1 on failure.
 */
int receive_control_packet(int socket, command_t *buffer, size_t maxlen, int wait, int server);

#endif /* USING_JINAMP_CTL */
#endif /* JINAMP_CONTROL_H */
