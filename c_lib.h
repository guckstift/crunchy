#include <stdio.h>

#define _CRUNCHY_DEBUG

#ifdef CRUNCHY_DEBUG
	#define debug printf
#else
	#define debug if(0)
#endif

typedef struct {
	int refs;
	int length;
	char data[1];
} string;

void* crunchy_malloc(int size);
void crunchy_free(void* ptr);
string* string_new(int length, char* source);
string* string_incref(string* str);
void string_soft_decref(string* str);
void string_decref(string* str);
string* string_assign(string* dest, string* src);
string* string_concat(string* left, string* right);
string *int_to_string(int number);

extern int num_mallocs;
