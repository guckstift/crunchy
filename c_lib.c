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

void* ref_incref(void* _r)
{
	ref* r = _r;
	
	if(r && r->refs != -1) {
		r->refs++;
		debug("incref %lu to %i\n", (unsigned long)r, r->refs);
	}
	
	return r;
}

void ref_soft_decref(void* _r)
{
	ref* r = _r;
	
	if(r && r->refs != -1) {
		if(r->refs > 0) {
			r->refs--;
			debug("decref %lu to %i\n", (unsigned long)r, r->refs);
		}
	}
}

void ref_decref(void* _r)
{
	ref* r = _r;
	
	if(r && r->refs != -1) {
		ref_soft_decref(r);
		
		if(r->refs == 0) {
			crunchy_free(r);
		}
	}
}

void* ref_assign(void* dest, void* src)
{
	ref_decref(dest);
	return ref_incref(src);
}

string* string_new(int length, char* source)
{
	string* str = crunchy_malloc(sizeof(string) - 1 + length);
	str->r.refs = 0;
	str->length = length;
	memcpy(str->data, source, length);
	return str;
}

string* string_concat(string* left, string* right)
{
	string* str;
	ref_incref(left);
	ref_incref(right);
	str = crunchy_malloc(sizeof(string) - 1 + left->length + right->length);
	str->r.refs = 0;
	str->length = left->length + right->length;
	memcpy(str->data, left->data, left->length);
	memcpy(str->data + left->length, right->data, right->length);
	ref_decref(left);
	ref_decref(right);
	return str;
}

string* string_concats(int opcount, ...)
{
	va_list args;
	int total_len = 0;
	string* str;
	int i, offs;
	
	va_start(args, opcount);
	
	for(i=0; i<opcount; i++) {
		string* part = va_arg(args, string*);
		ref_incref(part);
		total_len += part->length;
	}
	
	va_end(args);
	str = crunchy_malloc(sizeof(string) - 1 + total_len);
	str->r.refs = 0;
	str->length = total_len;
	va_start(args, opcount);
	
	for(i=0, offs=0; i<opcount; i++) {
		string* part = va_arg(args, string*);
		memcpy(str->data + offs, part->data, part->length);
		offs += part->length;
		ref_decref(part);
	}
	
	va_end(args);
	return str;
}

string* int_to_string(int number)
{
	char buf[16];
	int len = sprintf(buf, "%i", number);
	return string_new(len, buf);
}

string* float_to_string(double number)
{
	int len;
	char* buf = d2s(number, &len);
	return string_new(len, buf);
}

cr_bool string_equ(string* left, string* right)
{
	return left->length == right->length && memcmp(left->data, right->data, left->length) == 0;
}
