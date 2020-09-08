#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "^ 2 =";
int v0 = 0;
int main(int argc, char **argv) {
	while((v0!=10)) {
		int v1 = 0;
		v0 = (v0+1);
		v1 = (v0*v0);
		printf("%i ^ 2 = %i\n", v0, v1);
	}
	debug("num left allocs %i\n", num_mallocs);
}
