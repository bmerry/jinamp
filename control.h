/*
 $Id: control.h,v 1.1 2002/06/27 10:57:06 bruce Exp $

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

enum command_type_t {
  COMMAND_NEXT,
  COMMAND_STOP
};

/* any payload appears in an extended form of the data structure, in the same
 * way sockaddr_t is extended for various address formats.
 */
typedef struct {
  command_type_t command;
} command_t;

/* Creates a control socket with a given name (a path), opens it for listening
 * and returns the descriptor. Returns -1 on error.
 */
int create_control_socket(const char *name);

/* Sends the given packet to the socket, which must already have been bound
 * and connected to the server
 */
int send_control_packet(int socket, command_t *command, size_t command_len);

#endif /* JINAMP_CONTROL_H */
