#include <string.h>
#include <stdlib.h>
#include "c_lib.h"

int num_mallocs = 0;

void* crunchy_malloc(int size)
{
	debug("malloc\n");
	num_mallocs ++;
	return malloc(size);
}

void crunchy_free(void* ptr)
{
	debug("free\n");
	num_mallocs --;
	free(ptr);
}

string* string_new(int length, char* source)
{
	string* str = crunchy_malloc(sizeof(string) - 1 + length);
	str->refs = 0;
	str->length = length;
	memcpy(str->data, source, length);
	return str;
}

string* string_incref(string* str)
{
	if(str && str->refs != -1) {
		str->refs++;
		debug("incref %lu to %i\n", (unsigned long)str, str->refs);
	}
	
	return str;
}

void string_soft_decref(string* str)
{
	if(str && str->refs != 0x7fFFffFF) {
		if(str->refs > 0) {
			str->refs--;
			debug("decref %lu to %i\n", (unsigned long)str, str->refs);
		}
	}
}

void string_decref(string* str)
{
	if(str && str->refs != -1) {
		string_soft_decref(str);
		
		if(str->refs <= 0) {
			crunchy_free(str);
		}
	}
}

string* string_assign(string* dest, string* src)
{
	string_decref(dest);
	return string_incref(src);
}

string* string_concat(string* left, string* right)
{
	string* str = 0;
	string_incref(left);
	string_incref(right);
	str = crunchy_malloc(sizeof(string) - 1 + left->length + right->length);
	str->refs = 0;
	str->length = left->length + right->length;
	memcpy(str->data, left->data, left->length);
	memcpy(str->data + left->length, right->data, right->length);
	string_decref(left);
	string_decref(right);
	return str;
}

string *int_to_string(int number)
{
	char buf[16];
	int len = sprintf(buf, "%i", number);
	return string_new(len, buf);
}
