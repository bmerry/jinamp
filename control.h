/*
 $Id: control.h,v 1.3 2002/11/18 08:20:51 bruce Exp $

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
#if HAVE_SYS_UN_H
# define USING_JINAMP_CTL 1
#endif

#if USING_JINAMP_CTL
#include <sys/types.h>

typedef enum {
  COMMAND_NEXT,
  COMMAND_LAST,
  COMMAND_PAUSE,
  COMMAND_CONTINUE,
  COMMAND_STOP
} command_type_t;

/* any payload appears in an extended form of the data structure, in the same
 * way sockaddr_t is extended for various address formats.
 */
typedef struct {
  command_type_t command;
} command_t;

/* returns the socket ID on success, dies on failure. */
int get_control_socket(int server);

void close_control_socket(int sock);

/* Called automatically with atexit, but must be called manually
 * if an unnatural (signal-based) termination is imminent.
 */
void cleanup();

/* Sends the given packet to the socket, which must already have been bound
 * and connected to the server
 */
void send_control_packet(int socket, const command_t *command, size_t command_len);

/* returns the control packet if there is one, or NULL if the
 * queue is empty. Caller must free the memory. */
command_t *receive_control_packet(int socket);

#endif /* USING_JINAMP_CTL */
#endif /* JINAMP_CONTROL_H */
