#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#define create(t) \
	calloc(1, sizeof(t))

char *filename = 0;
size_t line = 0;
size_t pos = 0;
Project *project = 0;
Unit *unit = 0;
char *src = 0;
char *start = 0;
String *str_pool = 0;
size_t pool_count = 0;
Token *token = 0;
Scope *scope = 0;
Scope *global = 0;
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

void error_stmt(Stmt *stmt, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s:%lu:%lu: error: ", filename, stmt->line, stmt->pos);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}

void error_expr(Expr *expr, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s:%lu:%lu: error: ", filename, expr->line, expr->pos);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(1);
}
