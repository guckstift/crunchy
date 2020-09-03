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

char s0_buf[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "hello";
char s1_buf[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "world";
char s2_buf[] = "\x01\x00\x00\x00\x02\x00\x00\x00" "aw";
char s3_buf[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "Hello";
char s4_buf[] = "\x01\x00\x00\x00\x06\x00\x00\x00" " World";
char s5_buf[] = "\x01\x00\x00\x00\x01\x00\x00\x00" "!";
char s6_buf[] = "\x01\x00\x00\x00\x04\x00\x00\x00" "jkjd";
char s7_buf[] = "\x01\x00\x00\x00\x08\x00\x00\x00" "  asdfs ";
char s8_buf[] = "\x01\x00\x00\x00\x05\x00\x00\x00" "Danny";
char s9_buf[] = "\x01\x00\x00\x00\x06\x00\x00\x00" "Hello ";
char s10_buf[] = "\x01\x00\x00\x00\x04\x00\x00\x00" "Bye ";
char s11_buf[] = "\x01\x00\x00\x00\x02\x00\x00\x00" "!?";
string* v0 = 0;
string* v1 = 0;
string* v2 = 0;
string* v3 = 0;
int v4 = 9;
string* v5 = 0;
unsigned char v6 = 1;
int main(int argc, char **argv) {
	v0 = string_assign(v0, ((string*)s0_buf));
	v1 = string_assign(v1, ((string*)s1_buf));
	v2 = string_assign(v2, ((string*)s2_buf));
	v3 = string_assign(v3, string_concat(string_concat(v0, v1), v2));
	{string* c0 = string_incref(string_concat(string_concat(((string*)s3_buf), ((string*)s4_buf)), ((string*)s5_buf)));
	string* c1 = string_incref(string_concat(((string*)s6_buf), ((string*)s7_buf)));
	printf("%.*s %i %.*s\n", c0->length, c0->data, v4, c1->length, c1->data); 
	string_decref(c0);
	string_decref(c1);}
	v5 = string_assign(v5, ((string*)s8_buf));
	if(v6) {
		{string* c0 = string_incref(string_concat(string_concat(((string*)s9_buf), v5), ((string*)s5_buf)));
		printf("%.*s\n", c0->length, c0->data); 
		string_decref(c0);}
	} else {
		{string* c0 = string_incref(string_concat(string_concat(((string*)s10_buf), v5), ((string*)s11_buf)));
		printf("%.*s\n", c0->length, c0->data); 
		string_decref(c0);}
	}
	string_decref(v0);
	string_decref(v1);
	string_decref(v2);
	string_decref(v3);
	string_decref(v5);
}
