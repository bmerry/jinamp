/*
 $Id: control.c,v 1.1 2002/06/27 10:57:06 bruce Exp $

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
#include <sys/socket.h>
#include <sys/un.h>

#include <debug.h>
#include <control.h>

int create_control_socket(const char *name) {
  int unix_socket;
  struct sockaddr_un addr;

  if (strlen(name) >= sizeof(addr.sun_path) return -1;  /* avoid buffer overflow */
  unix_socket = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (unix_socket == -1) return -1;
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_family, name);
  if (bind(unix_socket, &addr, sizeof(addr)) == -1) {
    dprintf(DEBUG_CONTROL_ERRORS, "failed to bind control socket: %s", strerror(errno));
    close(unix_socket);
    return -1;
  }
  if (listen(unix_socket, 5) == -1) goto errors;

  return unix_socket;

errors:
  dprintf(DEBUG_CONTROL_ERRORS, "failure in create_control_socket: %s", strerror(errno));
  close(unix_socket);
  unlink(name);
  return -1;
}

int send_control_packet(int socket, command_t *command, size_t command_len) {

}
