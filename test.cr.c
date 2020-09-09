#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Hello";
char s4[] = "\xFF\xFF\xFF\xFF\x03\x00\x00\x00" "Hey";
char s5[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "FLOAT";
char s6[] = "\xFF\xFF\xFF\xFF\x02\x00\x00\x00" "Pi";
int main(int argc, char **argv) {
	{string* c0 = string_incref(string_concat(((string*)s3), int_to_string(0)));
	string* c1 = string_incref(string_concat(int_to_string(0), ((string*)s3)));
	string* c2 = string_incref(string_concat(((string*)s3), (1 ? ((string*)true_string) : ((string*)false_string))));
	string* c3 = string_incref(string_concat((0 ? ((string*)true_string) : ((string*)false_string)), ((string*)s4)));
	string* c4 = string_incref(string_concat(((string*)s5), float_to_string(0.51)));
	string* c5 = string_incref(string_concat(float_to_string(3.141), ((string*)s6)));
	printf("%.*s %.*s %.*s %.*s %.*s %.*s\n", c0->length, c0->data, c1->length, c1->data, c2->length, c2->data, c3->length, c3->data, c4->length, c4->data, c5->length, c5->data);
	string_decref(c0);
	string_decref(c1);
	string_decref(c2);
	string_decref(c3);
	string_decref(c4);
	string_decref(c5);}
	debug("num left allocs %i\n", num_mallocs);
}
