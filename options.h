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

/*
 * options.c provides an generic interface for command line and config file processing.
 * Configuration options are registered with callbacks to handle them.
 */

#ifndef JINAMP_OPTIONS_H
#define JINAMP_OPTIONS_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_GETOPT_LONG
# include <getopt.h>
#else
# ifndef no_argument
#  define no_argument 0
#  define required_argument 1
#  define optional_argument 2
# endif
#endif

struct parameter
{
    char short_option;         /* command line short option */
    char *long_option;         /* command line long option */
    char *config_option;       /* config file option */
    int has_arg;               /* same as for getopt_long(3) */
    void (*callback)(const char *argument, void *data);
    void *callback_data;       /* an arbitrary parameter to pass to the callback */
};

/* processes the command line with getopt. Returns optind (see getopt(3)). */
int options_cmdline(int argc, char * const argv[], struct parameter parms[]);

/* processes the named file with the config file parser (returns 0 if file not found */
int options_file(const char *filename, struct parameter parms[]);

#endif /* JINAMP_OPTIONS_H */
