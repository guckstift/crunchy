#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" "num1 =";
char s4[] = "\xFF\xFF\xFF\xFF\x08\x00\x00\x00" "; num2 =";
char s5[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" "Hello ";
char s6[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "!";
char s7[] = "\xFF\xFF\xFF\xFF\x0B\x00\x00\x00" "Is it you, ";
char s8[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "?";
char s9[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Danny";
char s10[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" "Thomas";
int v0 = 0;
double v1 = 0.0;
int v3 = 0;
string* v4 = ((string*)empty_string);
void v2(int v0, double v1) {
	printf("num1 = %i ; num2 = %s\n", v0, d2s(v1, 0));
}
string* v5(int v3, string* v4) {
	if((v3>10)) {
		v2(42, 3.141);
		{string* string_res = string_incref(string_concat(string_concat(((string*)s5), v4), ((string*)s6)));
		string_soft_decref(string_res);
		return string_res;}
	}
	{string* string_res = string_incref(string_concat(string_concat(((string*)s7), v4), ((string*)s8)));
	string_soft_decref(string_res);
	return string_res;}
}
int main(int argc, char **argv) {
	{string* c0 = string_incref(v5(0, ((string*)s9)));
	printf("%.*s\n", c0->length, c0->data);
	string_decref(c0);}
	{string* c0 = string_incref(v5(100, ((string*)s10)));
	printf("%.*s\n", c0->length, c0->data);
	string_decref(c0);}
	string_decref(v4);
	debug("num left allocs %i\n", num_mallocs);
}
