#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define create(type) \
	calloc(1, sizeof(type))

char *filename = 0;
size_t line = 0;
size_t pos = 0;
Project *project = 0;
Unit *unit = 0;
char *src = 0;
TokenList *tokens = 0;
Token *token = 0;
Scope *scope = 0;
int level = 0;
FILE *cfile = 0;

void error(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	if(filename)
		fprintf(stderr, "%s:%lu:%lu: error: ", filename, line, pos);
	else
		fprintf(stderr, "error: ");
	
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}

void error_at(Token *token, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s:%lu:%lu: error: ", filename, token->line, token->pos);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}
