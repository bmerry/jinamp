%{
/*
 $Id: parse.y,v 1.5 2002/11/18 14:06:55 bruce Exp $
 
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

#include <misc.h>
#include <options.h>
#include <stdio.h>
#include <string.h>

#define YYSTYPE char *

int yyerror(char *msg);
extern int yylex();

/* defined in options.c */
void config_callback(const char *name, const char *argument);
%}

%token WORD
%token EQUALS
%token EOL

%%

input:	/* empty */
	| input line
;

line:	'\n'
	| WORD EQUALS WORD '\n' { config_callback($1, $3); }
        | WORD '\n' { config_callback($1, NULL); }
;

%%

int yyerror(char *msg) {
  fprintf(stderr, "Parse error in config file\n");
  return 0;
}
