/*
 $Id: jinamp-ctl.c,v 1.6 2002/12/02 05:34:37 bruce Exp $

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
  fprintf(stderr, "replace\tReplace play list with command line arguments\n");
  exit(1);
}

void send_replace(int sock, int argc, const char *argv[]) {
  int i;
  int count, total;
  command_list_t rep;

  rep.command = COMMAND_REPLACE;
  total = 0;
  for (i = 0; i < argc; i++) {
    count = strlen(argv[i]) + 1;
    if (count + total > sizeof(rep.argv)) break;
    strcpy(rep.argv + total, argv[i]);
    total += count;
  }
  rep.argc = i;
  send_control_packet(sock, (command_t *) &rep, sizeof(rep) - sizeof(rep.argv) + total, 1);
}

int main(int argc, const char *argv[]) {
  int sock;
  int sent = 0;
  command_t msg;

  if (argc <= 1) show_usage();
  sock = get_control_socket(0);
  if (sock == -1) pdie("failed to open connection");
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
  else if (!strcmp(argv[1], "replace")) {
    send_replace(sock, argc - 2, argv + 2);
    sent = 1;
  }
  else show_usage();

  if (!sent)
    send_control_packet(sock, &msg, sizeof(msg), 1);
  close_control_socket(sock, 0);
  return 0;
}
