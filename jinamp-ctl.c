/*
 $Id: jinamp-ctl.c,v 1.2 2002/11/16 21:54:58 bruce Exp $

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

int main() {
  int sock;
  command_t msg;

  sock = get_control_socket(0);
  msg.command = COMMAND_LAST;
  send_control_packet(sock, &msg, sizeof(&msg));
  close_control_socket(sock);
  return 0;
}
