#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "A";
char s4[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "B";
char s5[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "C";
int v0 = (1-2+3);
string* v1 = ((string*)empty_string);
int main(int argc, char **argv) {
	printf("1\n");
	printf("%i\n", (1-2));
	v1 = string_assign(v1, string_concat(int_to_string(((1-2)+3)), ((string*)s3)));
	{string* c0 = string_incref(string_concat(string_concat(((string*)s3), ((string*)s4)), ((string*)s5)));
	printf("%.*s\n", c0->length, c0->data);
	string_decref(c0);}
	printf("%s\n", d2s((((double)(1+1))-2.2), 0));
	string_decref(v1);
	debug("num left allocs %i\n", num_mallocs);
}
