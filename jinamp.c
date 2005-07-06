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

#define _XOPEN_SOURCE 500
#if HAVE_GETOPT_LONG
# include <getopt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#if HAVE_LIMITS_H
# include <limits.h>
#else
# ifndef LONG_MAX
#  define LONG_MAX ((~(0L)) >> 1)
# endif
#endif
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#include <stddef.h>

#include <misc.h>
#include <songset.h>
#include <load.h>
#include <options.h>
#include <control.h>
#include <debug.h>

#ifndef DEFAULT_PLAYER
# define DEFAULT_PLAYER "/usr/local/bin/playaudio"  /* quick hacks */
#endif
#ifndef DEFAULT_DELAY
# define DEFAULT_DELAY 2
#endif
#ifndef DEFAULT_KILL_SIGNAL
# define DEFAULT_KILL_SIGNAL SIGINT
#endif
#ifndef DEFAULT_PAUSE_SIGNAL
# define DEFAULT_PAUSE_SIGNAL SIGSTOP
#endif
#ifndef CONFIG_SUFFIX
# define CONFIG_SUFFIX "/.jinamp"
#endif
#ifndef PLAYLIST_REGEX
# define PLAYLIST_REGEX ".*\\.lst"
#endif

/* dynamic data */
static pid_t playerpid = 0;
static struct songset *songs;
static int control_sock = -1;

/* config data */
static char *player;
static char *playlist_regex;
static char *exclude_regex;
static long delay = DEFAULT_DELAY;
static long count = -1, repeat = -1;
static enum songset_key key = KEY_RANDOM;
static int kill_signal = DEFAULT_KILL_SIGNAL;
static int pause_signal = DEFAULT_PAUSE_SIGNAL;

/* signal handling stuff */
static sigset_t blocked, unblocked;
static volatile int child_died;

static void cleanup(void)
{
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    if (control_sock != -1)
    {
        close_control_socket(control_sock, 1);
        control_sock = -1;
    }
}

/* Recursive function for parsing command line parameters. It allocates and fills in a
 * set starting at `first' and continuing until the end of the expression, which is
 * determined by level (a terminator is end of parameters or an unmatched ')').
 * 0: read until arguments run out (used at top level only)
 * 1: read until unmatched ')' seen, and discard it (for processing inside parentheses)
 * 2: read until terminator or a '!' or concatenation seen, leave it
 *
 * Note that in all levels, a subexpression does not count in terms of the termination
 * rules. Ok that sounds confusing, just look at the code.
 *
 * The index of the parameter AFTER the last one swallowed is returned in *end.
 * NULL is returned if the expression is invalid.
 */
static struct songset *read_argv(int argc, const char * const argv[],
                                 int first, int level, int *end,
                                 void *playlist_handle, void *exclude_handle,
                                 struct songset *old)
{
    int i;
    struct songset *current, *next, *done;
    int op;  /* 0 = union, 1 = subtract, 2 = intersect */

    current = NULL;
    op = 0;
    for (i = first; i < argc; i++)
    {
        next = NULL;
        if (argv[i][0] != '\0' && argv[i][1] == '\0')
        {
            switch (argv[i][0])
            {
            case '(':
                next = read_argv(argc, argv, i + 1, 1, &i, playlist_handle, exclude_handle, old);
                i--; /* since we will do i++ in a minute */
                if (!next) goto bailout;
                break;
            case '!':
            case '-':
                if (current == NULL) return NULL;    /* can't subtract from nothing */
                if (op != 0) goto bailout;           /* were expecting a set */
                if (level == 2)
                {
                    *end = i;
                    return current;
                }
                op = 1;
                continue;   /* this breaks out of the loop, not the switch */
            case '^':
                if (current == NULL) return NULL;    /* ^ is a binary op */
                if (op != 0) goto bailout;
                op = 2;
                continue;
            case ')':
                if (level == 0) goto bailout;
                if (op != 0) goto bailout;
                if (current == NULL) current = set_alloc(); /* handle () case */
                *end = i + ((level == 2) ? 0 : 1);
                return current;
            }
        }
        if (!next)
        {
            /* we only get here if nothing has been processed yet */
            if (level == 2 && op == 0 && current)         /* concatenation in level 2 */
            {
                *end = i;
                return current;
            }
            if (level < 2)
            {
                next = read_argv(argc, argv, i, 2, &i, playlist_handle, exclude_handle, old);
                if (!next) goto bailout;
                i--;
            }
            else if (argv[i][0] == '$' && argv[i][1] == '\0' && old)
            {
                next = set_alloc();
                set_merge(next, old);
            }
            else
            {
                done = set_alloc();
                next = set_alloc();
                read_object(argv[i], next, done,
                            playlist_handle, exclude_handle);
                set_free(done);
            }
        }
        switch (op)
        {
        case 0:
            if (current == NULL) current = next;
            else
            {
                set_merge(current, next);
                set_free(next);
            }
            break;
        case 1:
            set_subtract(current, next);
            set_free(next);
            op = 0;
            break;
        case 2:
            set_mask(current, next);
            set_free(next);
            op = 0;
            break;
        }
    }
    if (op != 0) goto bailout;
    if (level == 1) goto bailout;  /* should be terminated by ')' */
    *end = i;
    return current;

bailout:
    if (current) set_free(current);
    return NULL;
}

/* Take the command line options and expand to get playlist
 * `first' is the first argument to process (to work with getopt)
 */
static void populate(int argc, const char * const argv[], int first)
{
    void *playlist_handle, *exclude_handle;
    struct songset *old;

    old = songs;
    if (playlist_regex && *playlist_regex)
    {
        playlist_handle = regex_init(playlist_regex);
        if (playlist_handle == NULL)
            die("Invalid playlist regex: \"%s\"", playlist_regex);
    }
    else playlist_handle = NULL;
    if (exclude_regex && *exclude_regex)
    {
        exclude_handle = regex_init(exclude_regex);
        if (exclude_handle == NULL)
            die("Invalid exclude regex: \"%s\"", exclude_regex);
    }
    else exclude_handle = NULL;

    /* we overwrite first simply because it is no longer needed */
    songs = read_argv(argc, argv, first, 0, &first,
                      playlist_handle, exclude_handle, old);
    if (old) set_free(old);

    regex_done(playlist_handle);
    regex_done(exclude_handle);

    if (!songs)
    {
        fprintf(stderr, "The expression was invalid (see the man page)\n");
        cleanup();
        exit(1);
    }

    set_sort(songs, key);

    if (set_empty(songs))
    {
        fprintf(stderr, "No songs to play!\n");
        cleanup();
        exit(1);
    }
}

static void process_commands(void);

static void set_counts(void)
{
    struct song *i;

    if (count >= 0 && repeat == -1) repeat = 0;
    i = songs->head;
    do
    {
        i->repeat = repeat;
        i = i->next;
    } while (i != songs->head);
}

/* the main play loop */
static void play_all(void)
{
    pid_t f;
    long counter;

    counter = count;
    while (!set_empty(songs))
    {
        if (songs->head->repeat >= 0)
        {
            if (counter > 0) counter--;
            else if (songs->head->repeat > 0) songs->head->repeat--;
            else
            {
                set_erase(songs, songs->head);
                continue;
            }
        }

        f = fork();
        switch (f)
        {
        case -1: perror("jinamp: fork"); cleanup(); exit(2); break;
        case 0:
            dprintf(DBG_MISC, "exec'ing %s %s\n", player, songs->head->name);
#if DEBUG
            close(0);
            close(1);
            close(2);
            setsid();
#endif
            execl(player, player, songs->head->name, NULL);
            cleanup();
            exit(2);
            break;
        default:
            playerpid = f;
            process_commands();
            sleep(delay);
        }

        if (songs->head) songs->head = songs->head->next;
    }
}

static void command_last(void)
{
    set_dispose(songs);
}

static void command_next(void)
{
    if (playerpid)
    {
        kill(playerpid, kill_signal);
        kill(playerpid, SIGCONT); /* in case it was paused */
    }
}

static void command_pause(void)
{
    if (playerpid) kill(playerpid, pause_signal);
}

static void command_continue(void)
{
    if (playerpid) kill(playerpid, SIGCONT);
}

static void command_stop(void)
{
    command_next();
    cleanup();
    exit(0);
}

/* jinamp-ctl sends lists of strings as a flat list, one string right after the next
 * using NULLs as separators. This function splits this back into an argv-style array,
 * returning the number of parameters. This may be less than cmd->argc if the array is
 * exhausted first. Return -1 on error.
 */
static int unpack(const struct command_list_t *cmd, const char ***argv)
{
    int i;
    const char *cur, *nxt;

    cur = cmd->argv;
    *argv = malloc(cmd->argc * sizeof(char *));
    if (*argv == NULL)
    {
        dprintf(DBG_CONTROL_ERRORS, "Out of memory in unpack\n");
        return -1;
    }
    for (i = 0; i < cmd->argc; i++)
    {
        if (cur >= cmd->argv + sizeof(cmd->argv)) break;
        nxt = cur;
        while (nxt < cmd->argv + sizeof(cmd->argv) && *nxt != '\0')
            nxt++;
        if (nxt >= cmd->argv + sizeof(cmd->argv)) break;
        (*argv)[i] = cur;
        dprintf(DBG_CONTROL_DATA, "unpack %d: %s\n", i, cur);
        cur = nxt + 1;
    }
    return i;
}

static void dispatch_command(const struct command_t *cur)
{
    int argc;
    const char **argv;
    struct command_string_t reply;

    dprintf(DBG_CONTROL_DATA, "Got command %d\n", cur->command);
    switch (cur->command)
    {
    case COMMAND_WAKE: break; /* used internally by SIGCHLD handler to wake us up */
    case COMMAND_LAST: command_last(); break;
    case COMMAND_NEXT: command_next(); break;
    case COMMAND_PAUSE: command_pause(); break;
    case COMMAND_CONTINUE: command_continue(); break;
    case COMMAND_STOP: command_stop(); break;
    case COMMAND_REPLACE:
        argc = unpack((const struct command_list_t *) cur, &argv);
        if (argc != -1)
            populate(argc, argv, 0);
        break;
    case COMMAND_QUERY:
        reply.command = REPLY_QUERY;
        my_strncpy(reply.value, songs->head->name, sizeof(reply.value));
        send_control_packet(control_sock, (struct command_t *) &reply, (reply.value + strlen(reply.value) + 1) - (char *) &reply, 0, 0);
        break;
    default: abort();
    }
}

static void process_commands(void)
{
    struct command_t *cur;
    int count;
    int broken;

    while (1)
    {
        cur = (struct command_t *) malloc(sizeof(struct command_list_t)); /* FIXME: handle arbitrary sizes */
        if (control_sock == -1)
        {
            count = -1;
            sigsuspend(&unblocked);
        }
        else
        {
            sigprocmask(SIG_SETMASK, &unblocked, NULL);
            count = receive_control_packet(control_sock, cur, sizeof(struct command_list_t), 1, 1); /* FIXME: arb sizes */
            broken = 0;
            if (count == -1 && errno != EINTR) broken = 1;
            sigprocmask(SIG_SETMASK, &blocked, NULL);
            if (broken)
            {
                /* assume that the control socket is now broken and stop using it */
#ifdef DEBUG
                perror("error on control socket");
#endif
                close_control_socket(control_sock, 1);
                control_sock = -1;
            }
        }
        if (count >= 0)
            dispatch_command(cur);
        if (child_died)
        {
            while (waitpid(-1, NULL, WNOHANG) > 0);
            child_died = 0;
            free(cur);
            return;
        }
        free(cur);
    }
}

static RETSIGTYPE signalplayer(int sig)
{
    switch (sig)
    {
    case SIGUSR1: command_next(); break;  /* ogg123 prefers SIGINT to SIGTERM */
    case SIGTSTP: command_pause(); break; /* some players ignore TSTP */
    case SIGCONT: command_continue(); break;
    default: die("unexpected signal %d", sig);
    }
}

static RETSIGTYPE terminate(int sig)
{
    command_next();
    cleanup();
    raise(sig);
}

/* similar to terminate but leaves the player going */
static RETSIGTYPE fastquit(int sig)
{
    command_last();
}

static RETSIGTYPE sigchld(int sig)
{
    struct command_t wake;

    child_died = 1;
    /* Send a fake message. This will prevent a race condition on a
     * SIGCHLD arriving immediately before we start listening on the
     * control socket.
     */
    if (control_sock != -1)
    {
        wake.command = COMMAND_WAKE;
        send_control_packet(control_sock, &wake, sizeof(wake), 0, 1);
    }
}

static void setsigs(void)
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP;

    sigemptyset(&blocked);
    sigprocmask(SIG_BLOCK, &act.sa_mask, &unblocked); /* fetch current */
    sigprocmask(SIG_BLOCK, &act.sa_mask, &blocked);
    sigaddset(&blocked, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked, NULL);  /* block SIGCHLD */

    act.sa_handler = signalplayer;
    if (sigaction(SIGUSR1, &act, NULL) == -1) die("sigaction");
    if (sigaction(SIGTSTP, &act, NULL) == -1) die("sigaction");
    if (sigaction(SIGCONT, &act, NULL) == -1) die("sigaction");

    act.sa_handler = sigchld;
    if (sigaction(SIGCHLD, &act, NULL) == -1) die("sigaction");

    /* We would like to use SA_RESETHAND, but it is not standard.
     * Instead we reset the signal handlers manually in cleanup().
     */
    act.sa_handler = terminate;
    if (sigaction(SIGTERM, &act, NULL) == -1) die("sigaction");
    act.sa_handler = fastquit;
    if (sigaction(SIGINT, &act, NULL) == -1) die("sigaction");
    if (sigaction(SIGHUP, &act, NULL) == -1) die("sigaction");
}

/* Shows the help. The parameters are just to make it work as a callback */
static void show_help(const char *argument, void *data)
{
    printf(PACKAGE ": play files and lists specified on the command line\n\n");
    printf("Usage: " PACKAGE " [-p player] [-d delay] <options> <files>...\n");
    printf("Each set is a file, a directory or a playlist. The operators are\n");
    printf("- (subtraction), ^ (intersection) and parentheses.\n\n");
    printf("\t-p, --player\tOverride the default player [" DEFAULT_PLAYER "]\n");
    printf("\t-d, --delay\tOverride the default inter-song delay [%d]\n", DEFAULT_DELAY);
    printf("\t-c, --count\tNumber of songs to play (0 for infinite loop)\n");
    printf("\t-r, --repeat\tNumber of times to repeat all (0 for infinite)\n");
    printf("\t-n, --no-shuffle\tDo not shuffle the songs\n");
    printf("\t-x, --exclude\tSpecify files to ignore (extended regex)\n");
    printf("\t-L, --playlist\tSpecify extended regex for playlists\n");
    printf("\t-h, --help\tPrint this help text and exit\n");
    printf("\t-V, --version\tShow version information and exit\n");
    exit(0);
}

/* Shows the version and copyright. The parameters are just to make it work as a callback */
static void show_version(const char *argument, void *data)
{
    printf(PACKAGE " version " VERSION " Copyright 2001-2005 Bruce Merry\n\n");
    printf("You may use, modify and distribute this program under the terms of the\n");
    printf("GNU GPL version 2 only. See the file COPYING for more information.\n");
    exit(0);
}

/* data is a pointer to a character array. The string is freed if not
 * null, then allocated and populated with the argument.
 */
static void string_callback(const char *argument, void *data)
{
    char **str;
    if (argument != NULL && data != NULL)
    {
        str = (char **) data;
        if (*str) free(*str);
        *str = duplicate(argument);
    }
}

static void boolean_callback(const char *argument, void *data)
{
    *((int *) data) = 1;
}

static void invert_callback(const char *argument, void *data)
{
    *((int *) data) = 0;
}

static void delay_callback(const char *argument, void *data)
{
    char *check;

#if !HAVE_STRTOL
    delay = atoi(argument);
#else
    delay = (int) strtol(argument, &check, 10);
    if (*check)
    {
        printf("Delay is not a valid integer, using default delay of %d\n", DEFAULT_DELAY);
        delay = DEFAULT_DELAY;
    }
    if (delay == LONG_MAX && errno == ERANGE)
    {
        printf("Delay overflowed, using default delay of %d\n", DEFAULT_DELAY);
        delay = DEFAULT_DELAY;
    }
#endif
    if (delay < 0)
    {
        printf("Delay cannot be negative, using default delay of %d\n", DEFAULT_DELAY);
        delay = DEFAULT_DELAY;
    }
    dprintf(DBG_CONFIG_INFO, "Using delay: %ld\n", delay);
}

static void count_callback(const char *argument, void *data)
{
    char *check;

#if !HAVE_STRTOL
    count = atoi(argument);
#else
    count = (int) strtol(argument, &check, 10);
    if (*check)
    {
        printf("Count is not a valid integer, using infinite repeat\n");
        count = 0;
    }
    if (count == LONG_MAX && errno == ERANGE)
    {
        printf("Count overflowed, using infinite repeat\n");
        count = 0;
    }
#endif
    if (count < 0)
    {
        printf("Count cannot be negative, using infinite repeat\n");
        count = 0;
    }
    dprintf(DBG_CONFIG_INFO, "Using count: %ld\n", count);
}

static void repeat_callback(const char *argument, void *data)
{
    char *check;

#if !HAVE_STRTOL
    repeat = atoi(argument);
#else
    repeat = (int) strtol(argument, &check, 10);
    if (*check)
    {
        printf("Repeat is not a valid integer, using infinite repeat\n");
        repeat = 0;
    }
    if (repeat == LONG_MAX && errno == ERANGE)
    {
        printf("Repeat overflowed, using infinite repeat\n");
        repeat = 0;
    }
#endif
    if (repeat < 0)
    {
        printf("Repeat cannot be negative, using infinite repeat\n");
        repeat = 0;
    }
    dprintf(DBG_CONFIG_INFO, "Using repeat: %ld\n", repeat);
}

static void no_shuffle_callback(const char *argument, void *data)
{
    key = KEY_ORIGINAL;
}

/* FreeBSD documents but does not provide required_argument and
 * no_argument, so we have to use the numeric versions.
 */
static int options(int argc, char * const argv[])
{
    struct parameter opts[] =
    {
        {'p', "player", "player", 1, string_callback, &player},
        {'d', "delay", "delay", 1, delay_callback, NULL},
        {'h', "help", NULL, 0, show_help, NULL},
        {'c', "count", "count", 1, count_callback, NULL},
        {'r', "repeat", "repeat", 1, repeat_callback, NULL},
        {'n', "no-shuffle", "no-shuffle", 0, no_shuffle_callback, NULL},
        {'x', "exclude", "exclude", 1, string_callback, &exclude_regex},
        {'L', "playlist", "playlist", 1, string_callback, &playlist_regex},
        {'V', "version", NULL, 0, show_version, NULL},
        {0, 0, 0, 0, 0, 0}
    };
    char *home_dir;
    char *config;

    player = duplicate(DEFAULT_PLAYER);
    playlist_regex = duplicate(PLAYLIST_REGEX);
#ifdef EXCLUDE_REGEX
    exclude_regex = duplicate(EXCLUDE_REGEX);
#else
    exclude_regex = NULL;
#endif

    home_dir = getenv("HOME");
    if (home_dir)
    {
        config = safe_malloc(strlen(home_dir) + strlen(CONFIG_SUFFIX) + 2);
        sprintf(config, "%s/%s", home_dir, CONFIG_SUFFIX);
        options_file(config, opts);
        free(config);
    }
    return options_cmdline(argc, argv, opts);
}

int main(int argc, char * const argv[])
{
    int first;
#ifndef DEBUG
    int dev_null;
#endif

    /* if SUID is set, gain higher priority and drop root */
    if (geteuid() == 0)
    {
#if HAVE_SETPRIORITY
        setpriority(PRIO_PROCESS, 0, -15);
#endif
        setuid(getuid());
    }

    srand(time(NULL));
    /* Process the command line options */
    first = options(argc, argv);
    if (first >= argc)
        show_help(NULL, NULL);

    /* load the songs from the command line */
    if (first == 0) first = 1;
    populate(argc, (const char * const *) argv, first);

    /* background ourselves */
#ifndef DEBUG
    switch (fork())
    {
    case -1: perror("jinamp: fork"); cleanup(); exit(2);
    case 0:
#if HAVE_DUP2
        dev_null = open("/dev/null", O_RDWR);
        if (dev_null != -1)
        {
            dup2(dev_null, 0);
            dup2(dev_null, 1);
            dup2(dev_null, 2);
        }
        else
#endif
        {
            close(0);
            close(1);
            close(2);
        }
        setsigs();
        setsid();
        break;
    default: return 0;
    }
#else
    setsigs();
#endif

    /* the fun stuff */
    control_sock = get_control_socket(1);
#if HAVE_ATEXIT
    atexit(cleanup);
#endif
    set_counts();
    play_all();
    return 0;
}
