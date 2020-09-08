#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x0A\x00\x00\x00" "is a float";
string* v2 = ((string*)empty_string);
double v3 = 3.141592653589793;
string* v4 = ((string*)empty_string);
unsigned char v0() {
	
	return 1;
}
int v1() {
	
	return v0();
}
double v5() {
	
	return 0.4;
}
int main(int argc, char **argv) {
	v2 = string_assign(v2, int_to_string(v1()));
	printf("%.*s\n", v2->length, v2->data);
	printf("%s is a float\n", d2s(v3, 0));
	printf("0.3 is a float\n");
	v4 = string_assign(v4, float_to_string(v3));
	printf("%.*s\n", v4->length, v4->data);
	v4 = string_assign(v4, float_to_string(v5()));
	printf("%.*s\n", v4->length, v4->data);
	v3 = ((double)1);
	printf("%s\n", d2s(v3, 0));
	string_decref(v2);
	string_decref(v4);
	debug("num left allocs %i\n", num_mallocs);
}
