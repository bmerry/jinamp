/*
 $Id: jinamp-ctl.c,v 1.4 2002/11/18 14:02:17 bruce Exp $

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
#include <stdlib.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <misc.h>
#include <control.h>

void show_usage() {
  fprintf(stderr, "Usage: jinamp-ctl <command>\n\n");
  fprintf(stderr, "Commands are:\n");
  fprintf(stderr, "next\t\tskip remainer of current file\n");
  fprintf(stderr, "stop\t\tquit immediately\n");
  fprintf(stderr, "last\t\tquit at end of current file\n");
  fprintf(stderr, "pause\t\tPause current file\n");
  fprintf(stderr, "continue\tResume current file\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  int sock;
  command_t msg;

  sock = get_control_socket(0);
  if (sock == -1) pdie("failed to open connection");
  if (argc > 1) {
    if (!strcmp(argv[1], "next"))
      msg.command = COMMAND_NEXT;
    else if (!strcmp(argv[1], "last"))
      msg.command = COMMAND_LAST;
    else if (!strcmp(argv[1], "pause"))
      msg.command = COMMAND_PAUSE;
    else if (!strcmp(argv[1], "continue"))
      msg.command = COMMAND_CONTINUE;
    else if (!strcmp(argv[1], "stop"))
      msg.command = COMMAND_STOP;
    else show_usage();
  }
  else show_usage();

  send_control_packet(sock, &msg, sizeof(&msg));
  close_control_socket(sock);
  return 0;
}
