/*
 $Id$

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#define _XOPEN_SOURCE
#include <control.h>

#if USING_JINAMP_CTL

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <misc.h>

struct msgbuffer
{
    long mtype;
    char mtext[1];
};

int get_control_socket(int server)
{
    key_t key;
    const char *home;
    int oldid;

    home = getenv("HOME");
    if (home == NULL) home = "/";
    key = ftok(home, 'a');
    if (key == -1)
        key = ftok("/", 'a');
    if (key == -1) return -1;
    if (server)
    {
        /* first try to clean up old queues */
        oldid = msgget(key, 0600);
        if (oldid != -1)
            msgctl(oldid, IPC_RMID, NULL);
        return msgget(key, IPC_CREAT | IPC_EXCL | 0600);
    }
    else
        return msgget(key, 0600);
}

void close_control_socket(int sock, int server)
{
    if (server)
        msgctl(sock, IPC_RMID, NULL);
}

int send_control_packet(int socket, const command_t *command, size_t command_len, int wait, int toserver)
{
    struct msgbuffer *msg;
    int ret;
    int olderr;

    msg = malloc(command_len + sizeof(msg->mtype));
    if (msg == NULL) return -1;
    msg->mtype = toserver ? 1 : 2;
    memcpy(&msg->mtext, command, command_len);
    ret = msgsnd(socket, msg, command_len + sizeof(msg->mtype), wait ? 0 : IPC_NOWAIT);
    olderr = errno;
    free(msg);
    errno = olderr;
    return ret;
}

int receive_control_packet(int socket, command_t *buffer, size_t maxlen, int wait, int server)
{
    struct msgbuffer *msg;
    int ret;

    msg = (struct msgbuffer *) malloc(maxlen + sizeof(msg->mtype));
    ret = msgrcv(socket, msg, maxlen + sizeof(msg->mtype),
                 server ? 1 : 2, MSG_NOERROR | (wait ? 0 : IPC_NOWAIT));
    if (ret == -1) return -1;
    ret -= sizeof(msg->mtype);
    if (ret < 0) ret = 0;
    memcpy(buffer, msg->mtext, ret);
    return ret;
}

#else /* USING_JINAMP_CONTROL */

int get_control_socket(int server)
{
    return -1;
}

void close_control_socket(int sock, int server)
{
}

int send_control_packet(int socket, const command_t *command, size_t command_len, int wait, int toserver)
{
    return -1;
}

int receive_control_packet(int socket, command_t *buffer, size_t maxlen, int wait, int server)
{
    return -1;
}

#endif /* !USING_JINAMP_CONTROL */
