#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <crypt.h>
#include <arpa/inet.h>
#include "libcgi/libcgi.h"

/* should be globally declared as external */
extern struct cgienv *cgidata;

void show_main_page(void)
{
	cgi_show_header(BGCOLOR);
	printf("<center><table width=100%% height=100%% border=0><tr><td align=center valign=center>\n");
	printf("This is test CGI script that shows usage of LibCGI library.</td></tr>\n");
	printf("<tr><td align=center valign=middle><form method=get action=%s>\n", getenv("SCRIPT_NAME"));
	printf("<input type=hidden name=parameter1 value='value1'>");
	printf("<input type=text name=text value=''>");
	printf("<input type=text name=text value=''>");
	printf("<input type=hidden name=cmd value='gp'>");
	printf("<input type=submit value='GET form submit'></form></td></tr>\n");

	printf("<tr><td align=center valign=middle><form method=post action=%s>\n", getenv("SCRIPT_NAME"));
	printf("<input type=hidden name=parameter1 value='value1'>");
	printf("<input type=text name=text value=''>");
	printf("<input type=text name=text value=''>");
	printf("<input type=hidden name=cmd value='pp'>");
	printf("<input type=submit value='POST form submit'></form></td></tr>\n");

	printf("<tr><td align=center valign=middle>");
	printf("<form method=post action=%s enctype=\"multipart/form-data\">\n", getenv("SCRIPT_NAME"));
	printf("<input type=file name=file size=20><br>");
	printf("<input type=hidden name=parameter3 value='value3'>\n");
	printf("<input type=text name=text value=''>");
	printf("<input type=text name=text value=''>");
	printf("<input type=hidden name=cmd value='pp'><input type=submit value='multipart form POST submit'>");
	printf("</form></td></tr></table>\n");
	cgi_show_footer();
}

void show_get_form(void)
{
	cgi_show_header(BGCOLOR);
	printf("<center><table width=100%% height=100%% border=0><tr><td align=center valign=center>\n");
	printf("This page generated after using GET form submit method.</td></tr>\n");

	printf("<tr><td align=left valign=top><pre>\n");
#ifdef CGI_DEBUG
	cgi_treeprint();
#endif
	printf("</pre></td></tr>\n");

	printf("<tr><td align=center valign=center><form method=get action=%s>\n", getenv("SCRIPT_NAME"));
	printf("<input type=submit value='Return to main page'></form></td></tr></table>\n");
	cgi_show_footer();
}

void show_post_form(void)
{
	int counter = 0;
	FILE *out;

	/* if we get file, save it to disk (current working directory) */
	if(cgi_get_value("file", 0) && cgi_get_filename("file", 0))
	{
		out = fopen(cgi_get_filename("file", 0), "w");
		fwrite(cgi_get_value("file", 0), sizeof(char), cgi_get_length("file", 0), out);
		fclose(out);
	}

	cgi_show_header(BGCOLOR);
	printf("<center><table width=100%% height=100%% border=0><tr><td align=center valign=center>\n");
	printf("This page generated after using POST form submit method.</td></tr>\n");

	printf("<tr><td align=left valign=top><pre>\n");
	for(counter = 0; counter != cgi_get_values_qty("text"); counter++)
		printf("variable name: text value[%2d]: %s\n", counter, cgi_get_value("text", counter));
	printf("</pre></td></tr>\n");

	printf("<tr><td align=center valign=center><form method=get action=%s>\n", getenv("SCRIPT_NAME"));
	printf("<input type=submit value='Return to main page'></form></td></tr></table>\n");
	cgi_show_footer();
}

int main(int argc, char *argv[])
{
	int params = 0;

	/* assign NULL to structure initially */
	cgidata = NULL;

	/* first function we need to call - returns HTML form variables quantity */
	params = cgi_build_env();

	if(params)
	{
		/* this function returns value of HTML form variable "cmd" */
		char *command = cgi_get_value("cmd", 0);

		if(strcmp(command, "gp") == 0)
			show_get_form();
		else if(strcmp(command, "pp") == 0)
			show_post_form();
		else
			cgi_exit_error(BGCOLOR, "No default action for command \"%s\".", (command) ? command : "(NULL)");
	}
	else
		show_main_page();

	/* clean up allocated memory before exit - not critical, but... */
	cgi_cleanup();
	exit(0);
}
