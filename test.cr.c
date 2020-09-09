#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "H";
char s4[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "A";
int main(int argc, char **argv) {
	printf("%s\n", string_equ(string_concat(((string*)empty_string), ((string*)s3)), string_concat(((string*)s3), ((string*)empty_string))) ? "true" : "false");
	printf("%s\n", (string_equ(string_concat(((string*)empty_string), ((string*)s3)), string_concat(((string*)s4), ((string*)empty_string))) == 0) ? "true" : "false");
	debug("num left allocs %i\n", num_mallocs);
}
