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

char s0[] = "\x01\x00\x00\x00\x06\x00\x00\x00" "Global";
char s1[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "World";
char s2[] = "\x01\x00\x00\x00\x04\x00\x00\x00" "text";
char s3[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "Hello";
char s4[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "Danny";
string* v0 = 0;
int v6() {
	string* v4 = 0;
	if(1) {
		string* v1 = 0;
		string* v2 = 0;
		
		string_decref(v1);
		string_decref(v2);
		string_decref(v4);
		return 1;
		v1 = string_assign(v1, ((string*)s1));
		v2 = string_assign(v2, ((string*)s1));
		string_decref(v1);
		string_decref(v2);
	} else {
		string* v3 = 0;
		v3 = string_assign(v3, ((string*)s2));
		string_decref(v3);
	}
	v4 = string_assign(v4, ((string*)s3));
	while(0) {
		string* v5 = 0;
		v5 = string_assign(v5, ((string*)s4));
		string_decref(v5);
	}
	
	string_decref(v4);
	return 4;
	string_decref(v4);
}
int main(int argc, char **argv) {
	v0 = string_assign(v0, ((string*)s0));
	string_decref(v0);
}
