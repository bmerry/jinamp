/*
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

/*
 * options.c provides an generic interface for command line and config file processing.
 * Configuration options are registered with callbacks to handle them.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <options.h>
#include <misc.h>
#include <parse.h>
#include <debug.h>

#define LAST_PARM(x) (( \
  (x).short_option == 0 && \
  (x).long_option == 0 && \
  (x).config_option == 0))

int options_cmdline(int argc, char *argv[], struct parameter parms[]) {
  char *shortopts, *end;
#if HAVE_GETOPT_LONG
  struct option *longopts;
#endif
  int num_parms, i, j;
  int c;
  int long_return;

  num_parms = 0;
  while (!LAST_PARM(parms[num_parms])) num_parms++;
  dprintf(DBG_CONFIG_COUNT, "%d config parameters specified\n", num_parms);

  /* enough memory for every to have optional args (::) */
  shortopts = (char *) safe_malloc(3 * num_parms + 1);
  end = shortopts;
  for (i = 0; i < num_parms; i++)
    if (parms[i].short_option != 0) {
      *(end++) = parms[i].short_option;
      if (parms[i].has_arg != no_argument)
        *(end++) = ':';
#if !HAVE_GETOPT_LONG /* :: is a GNU extension, and we might not be using GNU */
      if (parms[i].has_arg == optional_argument)
        *(end++) = ':';
#endif
    }
  *end = '\0';                         /* add the terminator */

#if HAVE_GETOPT_LONG
  longopts = (struct option *) safe_malloc(sizeof(struct option) * (num_parms + 1));
  for (i = 0, j = 0; i < num_parms; i++) {
    if (parms[i].long_option != NULL) {
      longopts[j].name = parms[i].long_option;
      longopts[j].has_arg = parms[i].has_arg;
      longopts[j].flag = &long_return;
      longopts[j].val = i;
      j++;
    }
  }
  longopts[j].has_arg = 0;
  longopts[j].name = 0;
  longopts[j].flag = 0;
  longopts[j].val = 0;
#endif

  do {
#if HAVE_GETOPT_LONG
    c = getopt_long(argc, argv, shortopts, longopts, NULL);
#else
    c = getopt(argc, argv, shortopts);
#endif

    switch (c) {
    case 0: /* long option */
      parms[long_return].callback(optarg, parms[long_return].callback_data);
      break;
    case ':': /* missing argument */
    case '?': /* unknown option */
      exit(3);     /* getopt prints the error message */
    case -1: /* finished */
      break;
    default: /* short option */
      for (i = 0; i < num_parms; i++)
        if (c == parms[i].short_option) {
          parms[i].callback(optarg, parms[i].callback_data);
          break;
        }
      if (i >= num_parms)
        /* shouldn't get here... */
        die("getopt returned %c but no such option specified", c);
    }
  } while (c != -1);
  return optind;
}

/* stuff for config file handling */

/* FIXME: try to make this non-global somehow?
 * There isn't much point unless the parser is also re-entrant
 */
struct parameter *config_parms;

extern FILE *yyin;
extern int yyparse();

/* returns non-zero on success, zero if file not accessable. */
int options_file(char *filename, struct parameter parms[]) {
  config_parms = parms;
  yyin = fopen(filename, "r");
  if (yyin) {
    yyparse();
    fclose(yyin);
    return 1;
  }
  else return 0;
}

void config_callback(const char *name, const char *arg) {
  int i;
  for (i = 0; !LAST_PARM(config_parms[i]); i++)
    if (config_parms[i].config_option != NULL
        && !strcmp(config_parms[i].config_option, name)) {
      if (arg && config_parms[i].has_arg == no_argument)
        die("Configuration option %s should not have an argument.", name);
      if (!arg && config_parms[i].has_arg == required_argument)
        die("Configuration option %s should have an argument.", name);
      config_parms[i].callback(arg, config_parms[i].callback_data);
      return;
    }
  die("Invalid configuration option %s.", name);
}
