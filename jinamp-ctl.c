/*
 $Id: jinamp-ctl.c,v 1.9 2005/04/25 15:16:31 bruce Exp $

 jinamp: a command line music shuffler
 Copyright (C) 2001-2005  Bruce Merry.

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

#define _BSD_SOURCE
#define _XOPEN_SOURCE
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
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <misc.h>
#include <control.h>

static void show_usage(void)
{
    fprintf(stderr, "Usage: jinamp-ctl <command>\n\n");
    fprintf(stderr, "Commands are:\n");
    fprintf(stderr, "next\t\tskip remainer of current file\n");
    fprintf(stderr, "stop\t\tquit immediately\n");
    fprintf(stderr, "last\t\tquit at end of current file\n");
    fprintf(stderr, "pause\t\tPause current file\n");
    fprintf(stderr, "continue\tResume current file\n");
    fprintf(stderr, "replace\tReplace play list with command line arguments\n");
    fprintf(stderr, "query\tReturns the filename of the currently playing file\n");
    exit(1);
}

static void send_replace(int sock, int argc, const char *argv[])
{
    int i;
    size_t count, total;
    command_list_t rep;

    rep.command = COMMAND_REPLACE;
    total = 0;
    for (i = 0; i < argc; i++)
    {
        count = strlen(argv[i]) + 1;
        if (count + total > sizeof(rep.argv)) break;
        strcpy(rep.argv + total, argv[i]);
        total += count;
    }
    rep.argc = i;
    send_control_packet(sock, (command_t *) &rep, sizeof(rep) - sizeof(rep.argv) + total, 1, 1);
}

static RETSIGTYPE alarm_handler(int sig)
{
    fprintf(stderr, "Timed out\n");
    exit(2);
}

static void do_query(int sock)
{
    command_t query;
    command_string_t reply;

    query.command = COMMAND_QUERY;
    send_control_packet(sock, &query, sizeof(query), 1, 1);
    receive_control_packet(sock, (command_t *) &reply, sizeof(reply), 1, 0);
    reply.value[sizeof(reply.value) - 1] = '\0';
    printf("%s\n", reply.value);
}

static void prepare_alarm(void)
{
    struct sigaction act;

    act.sa_handler = alarm_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);

    alarm(2);
}

int main(int argc, const char *argv[])
{
    int sock;
    int sent = 0;
    command_t msg;

    prepare_alarm();
    if (argc <= 1) show_usage();
    sock = get_control_socket(0);
    if (sock == -1)
    {
        if (errno == ENOENT)
            die("failed to open connection: jinamp not running");
        else
            pdie("failed to open connection");
    }
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
    else if (!strcmp(argv[1], "replace"))
    {
        send_replace(sock, argc - 2, argv + 2);
        sent = 1;
    }
    else if (!strcmp(argv[1], "query"))
    {
        do_query(sock);
        sent = 1;
    }
    else show_usage();

    if (!sent)
        send_control_packet(sock, &msg, sizeof(msg), 1, 1);
    close_control_socket(sock, 0);
    return 0;
}
