#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
int main(int argc, char **argv) {
	printf("%s\n", d2s((((double)1)/((double)2)), 0));
	printf("%s\n", d2s((((double)1)/1.5), 0));
	printf("%s\n", d2s((0.5/((double)1)), 0));
	printf("%s\n", d2s((3.141/0.5), 0));
	debug("num left allocs %i\n", num_mallocs);
}
