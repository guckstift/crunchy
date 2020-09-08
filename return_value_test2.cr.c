#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Hello";
char s4[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "World";
char s5[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" " ";
char s6[] = "\xFF\xFF\xFF\xFF\x03\x00\x00\x00" "Foo";
char s7[] = "\xFF\xFF\xFF\xFF\x03\x00\x00\x00" "Bar";
string* v1() {
	string* v0 = ((string*)empty_string);
	v0 = string_assign(v0, string_concat(((string*)s3), ((string*)s4)));
	v0 = string_assign(v0, string_concat(v0, ((string*)s5)));
	{string* string_res = string_incref(string_concat(string_concat(v0, ((string*)s6)), ((string*)s7)));
	string_decref(v0);
	string_soft_decref(string_res);
	return string_res;}
	string_decref(v0);
}
int main(int argc, char **argv) {
	string_decref(v1());
	debug("num left allocs %i\n", num_mallocs);
}
