#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
string* v2 = ((string*)empty_string);
unsigned char v0() {
	
	return 1;
}
int v1() {
	
	return v0();
}
int main(int argc, char **argv) {
	v2 = string_assign(v2, int_to_string(v1()));
	printf("%.*s\n", v2->length, v2->data); 
	string_decref(v2);
	debug("num left allocs %i\n", num_mallocs);
}
