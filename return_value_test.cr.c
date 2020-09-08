#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Hello";
char s4[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "World";
char s5[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Danny";
string* v2 = ((string*)empty_string);
string* v1() {
	string* v0 = ((string*)empty_string);
	v0 = string_assign(v0, string_concat(((string*)s3), ((string*)s4)));
	{string* string_res = string_incref(v0);
	string_decref(v0);
	string_soft_decref(string_res);
	return string_res;}
	string_decref(v0);
}
int v3() {
	
	return 778;
}
int main(int argc, char **argv) {
	v2 = string_assign(v2, string_concat(v1(), ((string*)s5)));
	printf("%.*s\n", v2->length, v2->data);
	string_decref(v2);
	debug("num left allocs %i\n", num_mallocs);
}
