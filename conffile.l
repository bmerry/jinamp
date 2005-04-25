%{
/*
 $Id: conffile.l,v 1.6 2005/04/25 15:16:31 bruce Exp $

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
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>

#define YYSTYPE char *
#include <misc.h>
#include <parse.h>
#include <stdio.h>

#define INITIAL_SIZE 128

char *buffer = NULL;
int bufsize, buflen;

void resetbuffer()
{
    int i;
    if (!buffer)
    {
        buffer = safe_malloc(INITIAL_SIZE);
        bufsize = INITIAL_SIZE;
    }
    for (i = 0; i < bufsize; i++)
        buffer[i] = 0;
    buflen = 1;
}

void append(char c)
{
    char *old;

    if (buflen == bufsize)
    {
        old = buffer;
        bufsize *= 2;
        buffer = safe_malloc(bufsize);
        memset(buffer + buflen, 0, bufsize - buflen);
        memcpy(buffer, old, buflen);
        free(old);
    }
    buffer[buflen++ - 1] = c;
}

%}

%x QUOTED
TEXT	[A-Za-z0-9./_-]+

%%

#.*$		/* ignore comments */

\"		{ resetbuffer(); BEGIN(QUOTED); }

<QUOTED>\\.	{ append(yytext[1]); }
<QUOTED>[^"]	{ append(yytext[0]); }
<QUOTED>\"	{ yylval = duplicate(buffer); BEGIN(INITIAL); return WORD; }
<QUOTED><<EOF>>	{
			fprintf(stderr, "End of file found in quoted string\n");
			exit(1);
		}
{TEXT}		{ yylval = duplicate(yytext); return WORD; }
=		{ yylval = NULL; return EQUALS; }
[ \t]		/* remove whitespace */
\n		{ yylval = NULL; return '\n'; }

%%

int yywrap() 
{
    return 1;
}
