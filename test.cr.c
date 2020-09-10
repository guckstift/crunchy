#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x02\x00\x00\x00" "Jo";
char s4[] = "\xFF\xFF\xFF\xFF\x0C\x00\x00\x00" "Hello\nWorld";
int v0 = 0;
int v1 = 1;
int main(int argc, char **argv) {
	if(((v0==0)&&(v1==1))) {
		printf("Jo\n");
	}
	printf("Hello\nWorld\n");
	debug("num left allocs %i\n", num_mallocs);
}
