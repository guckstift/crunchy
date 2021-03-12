#define _GNU_SOURCE
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#define create(type) \
	calloc(1, sizeof(type))

char *filename = 0;
size_t line = 0;
size_t pos = 0;

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
