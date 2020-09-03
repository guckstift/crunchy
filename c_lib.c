#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	int refs;
	int length;
	char data[1];
} string;

string* string_incref(string* str)
{
	if(str) {
		str->refs++;
	}
	
	return str;
}

void string_decref(string* str)
{
	if(str) {
		str->refs--;
		
		if(str->refs == 0) {
			free(str);
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
	str = malloc(sizeof(string) - 1 + left->length + right->length);
	str->refs = 0;
	str->length = left->length + right->length;
	memcpy(str->data, left->data, left->length);
	memcpy(str->data + left->length, right->data, right->length);
	string_decref(left);
	string_decref(right);
	return str;
}
