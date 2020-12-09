/*
 * LibCGI - implementation of Common Gateway Interface
 * Copyright (C) 2001 - 2002,  Alexander B. Lavrinenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIBCGI_H	/* prevent multiply including of this file */
#define LIBCGI_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else	/* __STDC__ */
#include <varargs.h>
#endif	/* __STDC__ */

/* maximum length of simple query string parameter with consideration of encoded symbols */
#define PARAM_LENGTH	765
/* background color for HTML header and errors */
#define BGCOLOR		"#ffffff"
/* library name tag */
#define LIB_NAME	"LibCGI"
/* library version */
#define LIB_VERSION	"0.5.2"
/* character set used in CGI application */
#define DOC_CHARSET	"utf-8"
/* your CGI application name - used in HTML header and error page */
#define APP_NAME	"Inventory"
/* enables library debugging routines *
#define CGI_DEBUG
*/

#ifndef TRUE
#define TRUE		1
#endif	/* TRUE */
#ifndef FALSE
#define FALSE		0
#endif	/* FALSE */

/* HTML form variables and their values list */
struct cgienv
{
	char *cgivar;		/* CGI variable name */
	size_t **length;	/* length of CGI variable's value/file contents excluding trailing '\0' */
	char **filename;	/* file name(s) */
	char **mime_type;	/* file(s) contents */
	char **cgival;		/* CGI variable's value(s) */
	int quantity;		/* CGI variable's values quantity */
	struct cgienv *next;	/* next link pointer */
};

struct fd_retval
{
	char *data;
	int offset;
};

void	cgi_show_header(char *bgcolor);
void	cgi_show_footer(void);
void	cgi_cleanup(void);
#ifdef __STDC__
void	cgi_exit_error(char *bgcolor, char *reason, ...);
#else	/* __STDC__ */
void	cgi_exit_error(char *bgcolor, char *reason, va_dcl va_alist);
#endif	/* __STDC__ */
void	*cgi_xalloc(size_t size, char *obj_type);
size_t	cgi_get_length(char *var, int num);
char	*cgi_get_filename(char *var, int num);
char	*cgi_get_mimetype(char *var, int num);
char	*cgi_get_value(char *var, int num);
int	cgi_get_values_qty(char *var);
int	cgi_build_env(void);
#ifdef CGI_DEBUG
void	cgi_treeprint(void);
#endif	/* CGI_DEBUG */
#endif	/* LIBCGI_H */
