#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "A";
int v0 = (1-2+3);
string* v1 = ((string*)empty_string);
int main(int argc, char **argv) {
	v1 = string_assign(v1, string_concat(int_to_string(((1-2)+3)), ((string*)s3)));
	printf("%i\n", ((2*v0)-(3*v0)+1));
	string_decref(v1);
	debug("num left allocs %i\n", num_mallocs);
}
