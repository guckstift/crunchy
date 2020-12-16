#include <stdio.h>
#include <stdarg.h>

#define _CRUNCHY_DEBUG

#ifdef CRUNCHY_DEBUG
	#define debug printf
#else
	#define debug if(0)
#endif

typedef unsigned char cr_bool;

typedef struct {
	int refs;
} ref;

typedef struct {
	ref r;
	int length;
	char data[1];
} string;

typedef struct {
	ref r;
	int length;
	void *items;
} array;

void* crunchy_malloc(int size);
void crunchy_free(void* ptr);

void* ref_incref(void* r);
void ref_soft_decref(void* r);
void ref_decref(void* r);

#define ref_assign(dest, src) (ref_decref(dest), ref_incref(src))

string* string_new(int length, char* source);
string* string_concat(string* left, string* right);
string* string_concats(int opcount, ...);
string* int_to_string(int number);
string* float_to_string(double number);
cr_bool string_equ(string* left, string* right);
char *d2s(double f, int* len_out);

array* array_new();

extern int num_mallocs;
