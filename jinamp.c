/*
 $Id: jinamp.c,v 1.5 2002/01/07 04:27:39 bruce Exp $

 jinamp: a command line music shuffler
 Copyright (C) 2001  Bruce Merry.

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

#include <misc.h>
#include <list.h>
#include <load.h>
#include <options.h>

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
list *songs;
char **order;
int tot;

/* config data */
char *player;
char *playlist_regex;
char *exclude_regex;
int delay = DEFAULT_DELAY;
int count = 0, repeat = 0, do_shuffle = 1;
int kill_signal = DEFAULT_KILL_SIGNAL;
int pause_signal = DEFAULT_PAUSE_SIGNAL;

/* Recursive function for parsing command line parameters. It allocates and fills in a
 * list starting at `first' and continuing until the end of the expression, which is
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
list *read_argv(int argc, char *argv[], int first, int level, int *end,
               void *playlist_handle, void *exclude_handle) {
  int i;
  list *current, *next, *done;
  int op;  /* 0 = union, 1 = subtract, 2 = intersect */

  current = NULL;
  op = 0;
  for (i = first; i < argc; i++) {
    next = NULL;
    if (argv[i][0] != '\0' && argv[i][1] == '\0') {
      switch (argv[i][0]) {
      case '(':
        next = read_argv(argc, argv, i + 1, 1, &i, playlist_handle, exclude_handle);
        i--; /* since we will do i++ in a minute */
        if (!next) goto bailout;
        break;
      case '!':
      case '-':
        if (current == NULL) return NULL;    /* can't subtract from nothing */
        if (op != 0) goto bailout;           /* were expecting a list */
        if (level == 2) {
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
        if (current == NULL) current = list_alloc(); /* handle () case */
        *end = i + ((level == 2) ? 0 : 1);
        return current;
      }
    }
    if (!next) {
      /* we only get here if nothing has been processed yet */
      if (level == 2 && op == 0 && current) {       /* concatenation in level 2 */
        *end = i;
        return current;
      }
      if (level < 2) {
        next = read_argv(argc, argv, i, 2, &i, playlist_handle, exclude_handle);
        if (!next) goto bailout;
        i--;
      }
      else {
        done = list_alloc();
        next = list_alloc();
        read_object(argv[i], next, done, playlist_handle, exclude_handle);
        list_free(done, 1);
      }
    }
    switch (op) {
    case 0:
      if (current == NULL) current = next;
      else {
        list_merge(current, next);
        list_free(next, 1);
      }
      break;
    case 1:
      list_subtract(current, next);
      list_free(next, 1);
      op = 0;
      break;
    case 2:
      list_mask(current, next);
      list_free(next, 1);
      op = 0;
      break;
    }
  }
  if (op != 0) goto bailout;
  if (level == 1) goto bailout;  /* should be terminated by ')' */
  *end = i;
  return current;

  /* Ack! No! A goto! Bad Bruce. Ok this is necessary to prevent me writing the
   * same code 7 or 8 times, and can't be put in a procedure because it has a
   * return. */
bailout:
  if (current) list_free(current, 1);
  return NULL;
}

/* Take the command line options and expand to get playlist
 * `first' is the first argument to process (to work with getopt)
 */
void populate(int argc, char *argv[], int first) {
  void *playlist_handle, *exclude_handle;

  if (playlist_regex && *playlist_regex) {
    playlist_handle = regex_init(playlist_regex);
    if (playlist_handle == NULL)
      die("Invalid playlist regex: \"%s\"", playlist_regex);
  }
  else playlist_handle = NULL;
  if (exclude_regex && *exclude_regex) {
    exclude_handle = regex_init(exclude_regex);
    if (exclude_handle == NULL)
      die("Invalid exclude regex: \"%s\"", exclude_regex);
  }
  else exclude_handle = NULL;

  if (first == 0) first = 1;
  /* we overwrite first simply because it is no longer needed */
  songs = read_argv(argc, argv, first, 0, &first, playlist_handle, exclude_handle);

  regex_done(playlist_handle);
  regex_done(exclude_handle);

  if (!songs) {
    fprintf(stderr, "The expression was invalid (see the man page)\n");
    exit(1);
  }

  if (list_count(songs) == 0) {
    fprintf(stderr, "No songs to play!\n");
    exit(1);
  }
}

/* helper structure for use with the walker */
struct walker_data {
  int walker_pos;
  int *walker_map;
};

/* takes the data from the list and puts it in the array */
void shuffle_walker(char *item, void *data) {
  struct walker_data *cast_data = (struct walker_data *) data;
#if DEBUG
  printf("Walker: %s\n", item);
#endif
  order[cast_data->walker_map[(cast_data->walker_pos)++]] = item;
}

/* reorder the list and place in an array */
void shuffle() {
  int *used;
  int i, j, c;
  struct walker_data current;

  /* calculate random order */
  tot = list_count(songs);
#if DEBUG
  printf("Count: %d\n", tot);
#endif
  current.walker_map = (int *) safe_malloc(sizeof(int) * tot);
  used = (int *) safe_malloc(sizeof(int) * tot);
  for (i = 0; i < tot; i++)
    used[i] = 0;
  srand((unsigned int) time(NULL));
  for (i = 0; i < tot; i++) {
    if (do_shuffle)
      c = rand() % (tot - i) + 1;
    else
      c = 1;
    for (j = 0; c; j++)
      if (!used[j]) c--;
    current.walker_map[i] = --j;
    used[j] = 1;
  }

  /* extract into array */
  order = (char **) safe_malloc(sizeof(char *) * tot);
  current.walker_pos = 0;
  list_walk(songs, shuffle_walker, (void *) &current);

  /* free stuff that is no longer needed */
  free(current.walker_map);
  free(used);
  list_free(songs, 0);
}

/* this has to be global for the signal handlers to get at it */
pid_t playerpid = 0;

/* the main play loop */
void playall() {
  int i;
  char *cur;
  pid_t f;

  int counter = tot * repeat + count;
  for (i = 0;; i = (i + 1) % tot) {
    cur = order[i];
    f = fork();
    switch (f) {
    case -1: perror("jinamp: fork"); exit(2); break;
    case 0:
#if DEBUG
      close(0);
      close(1);
      close(2);
      setsid();
#endif
      execl(player, player, cur, NULL);
      exit(2);
      break;
    default:
      playerpid = f;
      while (waitpid(f, NULL, 0) == -1);
      playerpid = 0;
      sleep(delay);
    }
    if (counter > 0) {
      counter--;
      if (counter == 0) break;
    }
  }
}

RETSIGTYPE signalplayer(int sig) {
  int sendsig;
  if (playerpid) {
    switch (sig) {
    case SIGUSR1: sendsig = kill_signal; break;  /* ogg123 prefers SIGINT to SIGTERM */
    case SIGTSTP: sendsig = pause_signal; break; /* some players ignore TSTP */
    case SIGCONT: sendsig = SIGCONT; break;
    default: sendsig = kill_signal; break;       /* prevents -Wall from whining */
    }
    kill(playerpid, sendsig);
  }
  signal(sig, signalplayer);
}

RETSIGTYPE terminate(int sig) {
  if (playerpid)
    kill(playerpid, kill_signal);
  signal(sig, SIG_DFL);
  raise(sig);
}

void setsigs() {
  signal(SIGUSR1, signalplayer);
  signal(SIGTSTP, signalplayer);
  signal(SIGCONT, signalplayer);
  signal(SIGTERM, terminate);
}

/* Shows the help. The parameters are just to make it work as a callback */
void show_help(const char *argument, void *data) {
  printf(PACKAGE ": play files and lists specified on the command line\n\n");
  printf("Usage: " PACKAGE " [-p player] [-d delay] [-h] [-V] <set1> [!] <set2> [!] <set3>...\n");
  printf("Each set is a file, a directory or a playlist.\n");
  printf("Using ! negates the set (it may need to be escaped from the shell)\n\n");
  printf("\t-p, --player\tOverride the default player [" DEFAULT_PLAYER "]\n");
  printf("\t-d, --delay\tOverride the default inter-song delay [%d]\n", DEFAULT_DELAY);
  printf("\t-c, --count\tNumber of songs to play (0 for infinite loop)\n");
  printf("\t-r, --repeat\tNumber of times to repeat entire command line (0 for infinite)\n");
  printf("\t-x, --exclude\tSpecify files to ignore (extended regex)\n");
  printf("\t-L, --playlist\tSpecify extended regex for playlists\n");
  printf("\t-h, --help\tPrint this help text and exit\n");
  printf("\t-V, --version\tShow version information and exit\n");
  exit(0);
}

/* Shows the version and copyright. The parameters are just to make it work as a callback */
void show_version(const char *argument, void *data) {
  printf(PACKAGE " version " VERSION " Copyright 2001 Bruce Merry\n\n");
  printf("You may use, modify and distribute this program under the terms of the\n");
  printf("GNU GPL version 2 only. See the file COPYING for more information.\n");
  exit(0);
}

/* data is a pointer to a character array. The string is freed if not
 * null, then allocated and populated with the argument.
 */
void string_callback(const char *argument, void *data) {
  char **str;
  if (argument != NULL && data != NULL) {
    str = (char **) data;
    if (*str) free(*str);
    *str = duplicate(argument);
  }
}

void boolean_callback(const char *argument, void *data) {
  *((int *) data) = 1;
}

void invert_callback(const char *argument, void *data) {
  *((int *) data) = 0;
}

void delay_callback(const char *argument, void *data) {
  char *check;

#if !HAVE_STRTOL
  delay = atoi(argument);
#else
  delay = (int) strtol(argument, &check, 10);
  if (*check) {
    printf("Delay is not a valid integer, using default delay of %d\n", DEFAULT_DELAY);
    delay = DEFAULT_DELAY;
  }
  if (delay == LONG_MAX && errno == ERANGE) {
    printf("Delay overflowed, using default delay of %d\n", DEFAULT_DELAY);
    delay = DEFAULT_DELAY;
  }
#endif
  if (delay < 0) {
    printf("Delay cannot be negative, using default delay of %d\n", DEFAULT_DELAY);
    delay = DEFAULT_DELAY;
  }
#if DEBUG
  printf("Using delay: %d\n", delay);
#endif
}

void count_callback(const char *argument, void *data) {
  char *check;

#if !HAVE_STRTOL
  count = atoi(argument);
#else
  count = (int) strtol(argument, &check, 10);
  if (*check) {
    printf("Count is not a valid integer, using infinite repeat\n");
    count = 0;
  }
  if (delay == LONG_MAX && errno == ERANGE) {
    printf("Count overflowed, using infinite repeat\n");
    count = 0;
  }
#endif
  if (count < 0) {
    printf("Count cannot be negative, using infinite repeat\n");
    count = 0;
  }
#if DEBUG
  printf("Using count: %d\n", count);
#endif
}

void repeat_callback(const char *argument, void *data) {
  char *check;

#if !HAVE_STRTOL
  repeat = atoi(argument);
#else
  repeat = (int) strtol(argument, &check, 10);
  if (*check) {
    printf("Repeat is not a valid integer, using infinite repeat\n");
    repeat = 0;
  }
  if (delay == LONG_MAX && errno == ERANGE) {
    printf("Repeat overflowed, using infinite repeat\n");
    repeat = 0;
  }
#endif
  if (repeat < 0) {
    printf("Repeat cannot be negative, using infinite repeat\n");
    repeat = 0;
  }
#if DEBUG
  printf("Using repeat: %d\n", repeat);
#endif
}

int options(int argc, char *argv[]) {
  struct parameter opts[] = {
  {'p', "player", "player", required_argument, string_callback, &player},
  {'d', "delay", "delay", required_argument, delay_callback, NULL},
  {'h', "help", NULL, no_argument, show_help, NULL},
  {'c', "count", "count", required_argument, count_callback, NULL},
  {'r', "repeat", "repeat", required_argument, repeat_callback, NULL},
  {'n', "no-shuffle", "no-shuffle", no_argument, invert_callback, &do_shuffle},
  {'s', "shuffle", "shuffle", no_argument, boolean_callback, &do_shuffle},
  {'x', "exclude", "exclude", required_argument, string_callback, &exclude_regex},
  {'L', "playlist", "playlist", required_argument, string_callback, &playlist_regex},
  {'V', "version", NULL, no_argument, show_version, NULL},
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
  if (home_dir) {
    config = safe_malloc(strlen(home_dir) + strlen(CONFIG_SUFFIX) + 2);
    sprintf(config, "%s/%s", home_dir, CONFIG_SUFFIX);
    options_file(config, opts);
    free(config);
  }
  return options_cmdline(argc, argv, opts);
}

int main(int argc, char *argv[]) {
  int first;
  int dev_null;

  /* if SUID is set, gain higher priority and drop root */
  if (geteuid() == 0) {
#if HAVE_SETPRIORITY
    setpriority(PRIO_PROCESS, 0, -15);
#endif
    setuid(getuid());
  }

  /* Process the command line options */
  first = options(argc, argv);
  if (first >= argc)
    show_help(NULL, NULL);

  /* load the songs from the command line */
  populate(argc, argv, first);

  /* background ourselves */
#ifndef DEBUG
  switch (fork()) {
  case -1: perror("jinamp: fork"); exit(2);
  case 0:
    dev_null = open("/dev/null", O_RDWR);
    if (dev_null != -1) {
      dup2(dev_null, 0);
      dup2(dev_null, 1);
      dup2(dev_null, 2);
    }
    else {
      close(0);
      close(1);
      close(2);
    }
    setsigs();
    setsid(); break;
  default: return 0;
  }
#else
  setsigs();
#endif

  /* the fun stuff */
  shuffle();
  playall();
  return 0;
}
