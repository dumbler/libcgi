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

#include "libcgi.h"

extern char *strdup(const char *s);

/* list of HTML form variables' names and their values, parsed from CGI request string */
struct cgienv *cgidata;

/* puts HTML header and page beginning to standard output */
void cgi_show_header(char *bgcolor)
{
	printf("Content-Type: text/html\n\n<html><head>\n");
	printf("<meta http-equiv=Content-Type content=\"text/html; charset=%s\">\n", DOC_CHARSET);
	printf("<meta http-equiv=Author content=\"Alexander B. Lavrinenko\">\n");
	printf("<meta http-equiv=Pragma content=no-cache>\n");
	printf("<meta name=Generator content=\"%s Version %s\">\n", LIB_NAME, LIB_VERSION);
	printf("<meta name=Application content=\"%s\">\n", APP_NAME);
	printf("</head><body bgcolor=\"%s\" leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>\n", bgcolor);
}

/* puts HTML page end to standard output */
void cgi_show_footer(void)
{
	printf("</body></html>\n");
}

/* collects and cleans garbage before exit */
void cgi_cleanup(void)
{
	struct cgienv *link;
	int counter = 0;

	while(cgidata)
	{
		link = cgidata->next;
		for(counter = 0; counter != cgidata->quantity; counter++)
		{
			if(cgidata->cgival[counter])
				free(cgidata->cgival[counter]);
			if(cgidata->filename[counter])
				free(cgidata->filename[counter]);
			if(cgidata->mime_type[counter])
				free(cgidata->mime_type[counter]);
			if(cgidata->length[counter])
				free(cgidata->length[counter]);
		}
		if(cgidata->cgival)
			free(cgidata->cgival);
		if(cgidata->cgivar)
			free(cgidata->cgivar);
		if(cgidata->filename)
			free(cgidata->filename);
		if(cgidata->mime_type)
			free(cgidata->mime_type);
		if(cgidata)
			free(cgidata);
		cgidata = link;
	}
}

/* ends program gracefully with error message displayed in case of error */
#ifdef __STDC__
void cgi_exit_error(char *bgcolor, char *reason, ...)
#else
void cgi_exit_error(char *bgcolor, char *reason, va_dcl va_alist)
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, reason);
#else
	va_start(ap);
#endif
	cgi_show_header(bgcolor);

	printf("<center><table border=0 width=\"100%%\" height=\"100%%\" cellpadding=4 cellspacing=0>\n");
	printf("<tr><td width=\"100%%\" height=\"8%%\" align=\"center\" valign=\"middle\" colspan=2 bgcolor=\"#336699\">\n");
	printf("<font color=\"#ffffcc\" size=\"+2\">%s</font></td></tr>\n", APP_NAME);
	printf("<tr><td width=\"25%%\" align=\"center\" valign=\"middle\" bgcolor=\"#ffb5a0\">\n");
	printf("<font color=\"#cc0000\" size=\"+3\"><b>ERROR</b></font></td>\n");
	printf("<td width=\"75%%\" align=\"center\" valign=\"middle\"><p style=\"FONT: 12pt verdana, arial, helvetica;\">\n");
	printf("Application terminated while processing your request.<br>\n");
	printf("The following errors occured:</p><font color=\"#336699\">");
	vprintf(reason, ap);
	printf("</font>\n");
	printf("<p style=\"FONT: 12pt verdana, arial, helvetica;\">Please report this bug to author if you think\n");
	printf("this is wrong.</p></td></tr><tr><td width=\"25%%\" height=\"4%%\" valign=\"middle\" bgcolor=\"#336699\">\n");
	printf("<font color=\"#ffffcc\">%s Version %s</font></td>\n", LIB_NAME, LIB_VERSION);
	printf("<td width=\"75%%\" height=\"4%%\" align=\"right\" valign=\"middle\" bgcolor=\"#336699\">\n");
	printf("<font color=\"#ffffcc\">2001 - 2004 &copy; Alexander B. Lavrinenko</font></td>\n");
	printf("</tr></table></center>\n");
	cgi_show_footer();

	va_end(ap);
	cgi_cleanup();
	exit(1);
}

/* allocates memory for object with extra checks */
void *cgi_xalloc(size_t size, char *obj_type)
{
	void *object;

	if((object = malloc(size)) == NULL)
		cgi_exit_error(BGCOLOR, "Unable to allocate memory for %s.", obj_type);
	return(object);
}

/* searches for cgidata linked list link that contains variable var */
struct cgienv *find_link_by_cgivar(char *var)
{
	struct cgienv *link = cgidata;

	while(link)
	{
		if(!strcmp(var, link->cgivar))
			return(link);
		link = link->next;
	}
	return(NULL);
}

/* extracts boundary of combined CGI request from environment variable CONTENT_TYPE
 * that is represented in form "multipart/form-data; boundary=BOUNDARY". Returns
 * pointer to allocated string with boundary data in form "--BOUNDARY" */
char *cgi_parse_boundary(char *content_type_string)
{
	char *boundary = cgi_xalloc(strlen(content_type_string) - 27, "multipart boundary");
	int i = 0, e = 2;

	boundary[0] = '-';
	boundary[1] = '-';

	for(i = 0; i < strlen(content_type_string); i++)
	{
		if(i < 30)
			continue;
		boundary[e] = content_type_string[i];
		e++;
	}
	boundary[e] = '\0';

	return(boundary);
}

/* reads part of query string starting from offset up to symbols set end_marker
 * and places string read into allocated memory with proper string termination.
 * returns pointer to structure that contains pointer to allocated memory block
 * and length of data processed excluding '\0' symbol */
struct fd_retval *fill_data(char *query, int offset, char *end_marker)
{
	struct fd_retval *retval = (struct fd_retval *)cgi_xalloc(sizeof(struct fd_retval), "multipart data structure");
	int slider = 0, marker_len = strlen(end_marker);

	/* reset return data */
	retval->data = NULL;
	retval->offset = 0;

	/* at first define length of string needed in order to allocate proper amount of memory */
	for(slider = offset; strncmp(query + slider, end_marker, marker_len); slider++)
		retval->offset++;
	slider = 0;
	/* if string is of positive (non-zero) length, than */
	if(retval->offset)
	{
		/* allocate memory and fill it with data */
		retval->data = (char *)cgi_xalloc(retval->offset + 1, "CGI data");
		while(slider != retval->offset)
			retval->data[slider++] = query[offset++];
		retval->data[slider] = '\0';
	}
	return(retval);
}

/* processes and sorts combined query string that has length of length */
int cgi_process_multipart_query(char *query, int length, char *content_type_string)
{
	int params = 0, offset = 0, duplicate = FALSE;
	struct cgienv *instance = NULL, *step = NULL;
	struct fd_retval *m_data = NULL;
	char *boundary = cgi_parse_boundary(content_type_string);
	int bound_len = strlen(boundary) - 1;
	char *b_dupl = (char *)cgi_xalloc(bound_len + 4, "boundary duplicate");

	/* used as end of file/CGI variable's value identificator (CRLF) */
	sprintf(b_dupl, "\r\n%s", boundary);

	/* loops until query string of length is not processed fully */
	do
	{
		/* compare boundaries */
		if(strncmp(query + offset, boundary, bound_len))
			cgi_exit_error(BGCOLOR, "Announced and current boundaries differ.");
		/* shift pointer for the length of boundary (+1 is mystique, but works ;-) */
		offset += bound_len + 1;
		/* break loop if current boundary is the last one */
		if(!strncmp(query + offset, "--", 2))
			break;
		/* check whether Content-Disposition exists */
		if(strncmp(query + offset, "\r\nContent-Disposition: form-data; name=", 39))
			cgi_exit_error(BGCOLOR, "Invalid Content-Disposition in container.");
		/* skip "\r\nContent-Disposition: form-data; name="" string */
		offset += 40;
		/* define CGI variable name */
		m_data = fill_data(query, offset, "\"");
		/* if this variable exists already, assign its' structure address to instance */
		if((instance = find_link_by_cgivar(m_data->data)))
		{
			duplicate = TRUE;
			instance->quantity += 1;
			/* allocate memory for pointers for file names, MIME types and CGI variable values */
			if(!(instance->cgival = (char **)realloc(instance->cgival, sizeof(char *) * instance->quantity)))
				cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for CGI value.");
			if(!(instance->filename = (char **)realloc(instance->filename, sizeof(char *) * instance->quantity)))
				cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for file name.");
			if(!(instance->mime_type = (char **)realloc(instance->mime_type, sizeof(char *) * instance->quantity)))
				cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for MIME type.");
			if(!(instance->length = (size_t **)realloc(instance->length, sizeof(size_t *) * instance->quantity)))
				cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for file length.");
		}
		else
		{
			/* allocate memory for current linked list element */
			instance = (struct cgienv *)cgi_xalloc(sizeof(struct cgienv), "CGI environment structure");
			instance->next = NULL;
			instance->quantity = 1;
			instance->cgivar = m_data->data;
			instance->length = (size_t **)cgi_xalloc(sizeof(size_t *), "CGI values/files length array.");
			instance->cgival = (char **)cgi_xalloc(sizeof(char *), "CGI values array.");
			instance->filename = (char **)cgi_xalloc(sizeof(char *), "CGI file names array.");
			instance->mime_type = (char **)cgi_xalloc(sizeof(char *), "CGI MIME types array.");
		}
		/* shift pointer for the length of CGI variable name + double quote */
		offset += m_data->offset + 1;
		/* each call to fill_data allocates memory for this structure */
		free(m_data);
		/* if this part of query contains file */
		if(!strncmp(query + offset, "; filename=", 11))
		{
			/* shift pointer for the length of "; filename="" */
			offset += 12;
			/* get file name */
			m_data = fill_data(query, offset, "\"");
			/* if file name is not defined (empty field) leave it NULL */
			if(m_data->data)
				instance->filename[instance->quantity - 1] = m_data->data;
			else
				instance->filename[instance->quantity - 1] = NULL;
			/* shift pointer for the length of file name + double quote */
			offset += m_data->offset + 1;
			free(m_data);
			/* not all browsers provide Content-Type for file thus the following
			 * check is not mandatory (e.g., links does not provide this string) */
			if(!strncmp(query + offset, "\r\nContent-Type: ", 16))
			{
				offset += 2;
				 /* since MIME type can be any of known types and they vary in
				  * length we can't just shift pointer for the length of it -
				  * we need to read the whole string and determine its' length */
				m_data = fill_data(query, offset, "\r\n");
				/* if browser supplied file MIME type get it stored */
				if(m_data->data)
					instance->mime_type[instance->quantity - 1] = m_data->data;
				else
					instance->mime_type[instance->quantity - 1] = NULL;
				/* shift pointer for the length of Content-Type */
				offset += m_data->offset;
				free(m_data);
			}
		}

		/* TODO ? Does any browser support multiple files upload via ONE input form ?
		 * If someone REALLY needs this code, email me at lavrinenko_alex@mail.ru
		else if(!strncmp(query + offset, "\r\nContent-Type: ", 16))
		{
			here go multipart/mixed data with their own boundary
		} */

		/* shift pointer to "\r\n" after Content-Disposition string and "\r\n"
		 * that separate variable's value/file contents */
		offset += 4;
		/* if variable's value or file are not defined that here should be "\r\n"
		 * and boundary. Check for data absence */
		if(strncmp(query + offset, b_dupl, strlen(b_dupl) - 1))
		{
			/* if data/file contents exsist select them */
			m_data = fill_data(query, offset, b_dupl);
			/* store "variable value" pair in structure */
			if(m_data->data)
				instance->cgival[instance->quantity - 1] = m_data->data;
			else
				instance->cgival[instance->quantity - 1] = NULL;
			/* store variable's value length (file length) */
			instance->length[instance->quantity - 1] = (size_t *)cgi_xalloc(sizeof(size_t), "CGI value length.");
			*(instance->length[instance->quantity - 1]) = (size_t)(m_data->offset);
			/* shift pointer for the file length/variable's value length */
			offset += m_data->offset;
			free(m_data);
		}
		/* shift pointer to the trailing "\r\n" symbols, now query + offset
		 * points to next content boundary */
		offset += 2;

		/* move forward in linked list */
		if(!cgidata)
			cgidata = step = instance;
		else if(!duplicate)
		{
			step->next = instance;
			step = step->next;
		}
		/* increase parameters' quantity counter */
		params++;
		/* reset flag that points to another value of already exsisting variable */
		duplicate = FALSE;
	}
	while(offset != length + 1);
	/* free memory allocated for unnecessary boundary copy */
	free(b_dupl);
	return(params);
}

/* processes and sorts simple CGI query string of length */
int cgi_process_plain_query(char *query, int length)
{
	char *buf = "", tmp[PARAM_LENGTH], symbol[3], c;
	int params = 0, i = 0, e = 0, isval = 0, duplicate = FALSE;
	struct cgienv *instance = NULL, *step = NULL;

	/* initialize variables */
	symbol[2] = tmp[0] = '\0';

	while(i != (length + 1))
	{
		/* get next symbol from query string */
		buf = query++;
		/* check it for extra symbols presence */
		switch(*buf)
		{
			/* terminate with error if it is newline */
			case '\n':
				cgi_exit_error(BGCOLOR, "Wrong request string - newline detected.");
			/* get CGI variable name if it is symbol "=" */
			case '=':
				c = '\0';
				isval = 0;
				break;
			/* convert symbol to space if it is "+" */
			case '+':
				c = ' ';
				break;
			/* get value if this symbol is "&" or end of string */
			case '&':
			case '\0':
				c = '\0';
				isval = 1;
				break;
			/* next 2 symbols after "%" are in hexadecimal - convert them */
			case '%':
				/* copy them to separate string */
				strncpy(symbol, query, 2);
				/* if there's less then 2 symbols after "%" terminate with error */
				if(strlen(symbol) < 2)
					cgi_exit_error(BGCOLOR, "Wrong query string.");
				/* convert them to proper symbol */
				c = (char)strtoul(symbol, NULL, 16);
				/* reset temporary string */
				symbol[0] = '\0';
				/* shift query pointer to 2 unread symbols ahead */
				query += 2;
				/* and update counter */
				i += 2;
				break;
			default:
				c = (*buf);
		}
		/* if symbol is not '\0' add it to string */
		if(c != '\0')
		{
			if(e >= PARAM_LENGTH)
				cgi_exit_error(BGCOLOR, "CGI parameter is too long.");
			tmp[e] = c;
			e++;
		}
		else
		{
			/* if we got value of CGI variable */
			if(isval)
			{
				/* if there's no name of CGI variable then terminate with error */
				if(!instance || !instance->cgivar)
					cgi_exit_error(BGCOLOR, "Wrong query string - variable name is absent.");
				tmp[e] = c;
				e = 0;
				/* allocate memory for CGI variable values pointers */
				if(duplicate)
				{
					instance->quantity += 1;
					if(!(instance->cgival = (char **)realloc(instance->cgival, sizeof(char *) * instance->quantity)))
						cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for CGI value.");
					if(!(instance->length = (size_t **)realloc(instance->length, sizeof(size_t *) * instance->quantity)))
						cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for file length.");
					if(!(instance->filename = (char **)realloc(instance->filename, sizeof(char *) * instance->quantity)))
						cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for file name.");
					if(!(instance->mime_type = (char **)realloc(instance->mime_type, sizeof(char *) * instance->quantity)))
						cgi_exit_error(BGCOLOR, "Unable to reallocate memory block for MIME type.");
				}
				else
				{
					instance->quantity = 1;
					instance->length = (size_t **)cgi_xalloc(sizeof(size_t *), "CGI values length array.");
					instance->cgival = (char **)cgi_xalloc(sizeof(char *), "CGI values array.");
					instance->filename = (char **)cgi_xalloc(sizeof(char *), "CGI file names array.");
					instance->mime_type = (char **)cgi_xalloc(sizeof(char *), "CGI MIME types array.");
				}
				instance->filename[instance->quantity - 1] = NULL;
				instance->mime_type[instance->quantity - 1] = NULL;
				/* get "variable value" pair into structure */
				if(tmp[0] != '\0')
					/* UPDATE: 2004-03-26 17:48 changed from cgi_installstr() to strdup() */
					instance->cgival[instance->quantity - 1] = strdup(tmp);
				else
					instance->cgival[instance->quantity - 1] = NULL;
				/* save length of variable's value */
				instance->length[instance->quantity - 1] = (size_t *)cgi_xalloc(sizeof(size_t), "CGI value length.");
				*(instance->length[instance->quantity - 1]) = (size_t)(strlen(tmp));
				tmp[0] = '\0';
				isval = 0;

				/* move forward in linked list */
				if(!cgidata)
					cgidata = step = instance;
				else if(!duplicate)
				{
					step->next = instance;
					step = step->next;
				}
				/* reset flag that points to another value of already exsisting variable */
				duplicate = FALSE;
			}
			/* if this is the name of CGI variable */
			else
			{
				tmp[e] = c;
				e = 0;
				/* if this variable exists already, assign its' structure address to instance */
				if((instance = find_link_by_cgivar(tmp)))
					duplicate = TRUE;
				else
				{
					/* allocate memory for current linked list element */
					instance = (struct cgienv *)cgi_xalloc(sizeof(struct cgienv), "CGI environment structure");
					instance->cgival = instance->filename = instance->mime_type = NULL;
					instance->quantity = 0;
					instance->length = NULL;
					instance->next = NULL;
					/* UPDATE: 2004-03-26 17:52 changed from cgi_installstr() to strdup() */
					instance->cgivar = strdup(tmp);
				}
				tmp[0] = '\0';
				/* increase parameters counter */
				params += 1;
			}
		}
		i++;
	}
	/* return quantity of parameters processed */
	return(params);
}

/* builds CGI environment, returns the quantity of parameters received */
int cgi_build_env(void)
{
	char *query = NULL, *buf = NULL;
	int length = 0, params = 0;

	/* get CGI query type by request method */
	if((buf = getenv("REQUEST_METHOD")) == NULL)
		cgi_exit_error(BGCOLOR, "Unknown request method or unset variable \"REQUEST_METHOD\".");
	/* if this is GET */
	if(!strcmp(buf, "GET"))
	{
		/* and we got query string */
		if((query = getenv("QUERY_STRING")) == NULL)
			cgi_exit_error(BGCOLOR, "Query string environment is not set.");
		if((length = strlen(query)) != 0)
			/* process it and get quantity of parameters in it */
			params = cgi_process_plain_query(query, length);
	}
	/* if this is POST */
	else if(!strcmp(buf, "POST"))
	{
		/* content type */
		char *c_type = NULL;

		/* get query content type */
		if((c_type = getenv("CONTENT_TYPE")) == NULL)
			cgi_exit_error(BGCOLOR, "Unknown content type or unset variable \"CONTENT_TYPE\".");
		/* get query content length */
		if((buf = getenv("CONTENT_LENGTH")) == NULL)
			cgi_exit_error(BGCOLOR, "Unknown content length or unset variable \"CONTENT_LENGTH\".");
		length = strtol(buf, NULL, 0);

		/* allocate memory for query string */
		query = cgi_xalloc(sizeof(char) * length + 1, "query string");
		/* read relevant amount of bytes from standard input */
		if((fread(query, sizeof(char), length, stdin)) == length)
		{
#ifdef CGI_DEBUG
			FILE *query_out;
#endif	/* CGI_DEBUG */

			/* end query string properly */
			query[length] = '\0';
#ifdef CGI_DEBUG
			if((query_out = fopen("/var/tmp/CGI.out", "w")) == NULL)
				cgi_exit_error(BGCOLOR, "Unable to write debug log file!");
			fwrite(query, sizeof(char), length, query_out);
			fclose(query_out);
#endif	/* CGI_DEBUG */
			/* process it and get parameters quantity */
			if(!strcmp(c_type, "application/x-www-form-urlencoded"))
				params = cgi_process_plain_query(query, length);
			else if(!strncmp(c_type, "multipart/form-data; boundary=", 30))
				params = cgi_process_multipart_query(query, length, c_type);
			else
				cgi_exit_error(BGCOLOR, "The following content type is unsupported: %s.", (c_type) ? c_type : "(null content type)");
			free(query);

		}
		else
			cgi_exit_error(BGCOLOR, "String provided is shorter than announced length.");
	}
	/* if we failed to get or recognize request method terminate with error */
	else
		cgi_exit_error(BGCOLOR, "This method is currently unsupported.");

	/* return the quantity of parameters received */
	return(params);
}

/* returns quantity of values for CGI variable var */
int cgi_get_values_qty(char *var)
{
	/* create local copy of structure pointer */
	struct cgienv *link = cgidata;
	int found = 0;

	/* do until structure is not NULL and found flag not set */
	while((!found) && (link))
	{
		/* leave loop if value is found */
		if(!strcmp(var, link->cgivar))
		{
			found = 1;
			break;
		}
		/* move forward in linked list otherwise */
		link = link->next;
	}
	/* return quantity of values if one found */
	if(found)
		return(link->quantity);
	return(0);
}

/* returns pointer to value of CGI variable var with index num. CGI application
 * developer should control values quantity single CGI variable him/herself.
 * In case of wrong num value behaviour is UNDEFINED and there is no
 * guarantee that NULL is returned */
char *cgi_get_value(char *var, int num)
{
	/* create local copy of structure pointer */
	struct cgienv *link = cgidata;
	int found = 0;

	/* do until structure is not NULL and found flag not set */
	while((!found) && (link))
	{
		/* leave loop if value is found */
		if(!strcmp(var, link->cgivar))
		{
			found = 1;
			break;
		}
		/* move forward in linked list otherwise */
		link = link->next;
	}
	/* return value if one found */
	if(found)
		return(link->cgival[num]);
	return(NULL);
}

/* returns length of variable's value/file with name defined in var */
size_t cgi_get_length(char *var, int num)
{
	/* create local copy of structure pointer */
	struct cgienv *link = cgidata;
	int found = 0;

	/* do until structure is not NULL and found flag not set */
	while((!found) && (link))
	{
		/* leave loop if value is found */
		if(!strcmp(var, link->cgivar))
		{
			found = 1;
			break;
		}
		/* move forward in linked list otherwise */
		link = link->next;
	}
	/* return length of value if one found */
	if(found)
		return(*(link->length[num]));
	return((size_t)0);
}

/* returns pointer to file name for CGI variable var */
char *cgi_get_filename(char *var, int num)
{
	/* create local copy of structure pointer */
	struct cgienv *link = cgidata;
	int found = 0;

	/* do until structure is not NULL and found flag not set */
	while((!found) && (link))
	{
		/* leave loop if value is found */
		if(!strcmp(var, link->cgivar))
		{
			found = 1;
			break;
		}
		/* move forward in linked list otherwise */
		link = link->next;
	}
	/* return pointer to file name if one found */
	if(found)
		return(link->filename[num]);
	return(NULL);
}

/* returns pointer to file MIME type for CGI variable var */
char *cgi_get_mimetype(char *var, int num)
{
	/* create local copy of structure pointer */
	struct cgienv *link = cgidata;
	int found = 0;

	/* do until structure is not NULL and found flag not set */
	while((!found) && (link))
	{
		/* leave loop if value is found */
		if(!strcmp(var, link->cgivar))
		{
			found = 1;
			break;
		}
		/* move forward in linked list otherwise */
		link = link->next;
	}
	/* return pointer to MIME type if one found */
	if(found)
		return(link->mime_type[num]);
	return(NULL);
}

/* prints every "variable value" pair for diagnostics purposes
 * WARNING: in case of file transfer its' contents will be displayed
 * in browser AS IS */
#ifdef CGI_DEBUG
void cgi_treeprint(void)
{
	struct cgienv *link = cgidata;
	int counter = 0;
	char *none = "NONE";

	while(link)
	{
		printf("Variable: %s Quantity: %d\n", link->cgivar, link->quantity);
		for(counter = 0; counter != link->quantity; counter++)
		{
			printf("\t[%4.4d] Filename: %s Length: %d MIME Type: %s Value: ", counter,
			link->filename[counter] ? link->filename[counter] : none,
			link->length[counter] ? *(link->length[counter]) : 0,
			link->mime_type[counter] ? link->mime_type[counter] : none);
			if(link->cgival[counter])
				fwrite(link->cgival[counter], sizeof(char), *(link->length[counter]), stdout);
			printf("\n");
		}
		link = link->next;
	}
}
#endif	/* CGI_DEBUG */
