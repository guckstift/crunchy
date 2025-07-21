#include <stdint.h>
#include <stdio.h>

typedef struct {
	char *chars;
	int64_t length;
} String;

void print_string(String str)
{
	fwrite(str.chars, 1, str.length, stdout);
	printf("#\n");
}