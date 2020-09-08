#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" "Global";
char s4[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "World";
char s5[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "text";
char s6[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Hello";
char s7[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Danny";
string* v0 = ((string*)empty_string);
int v6() {
	string* v4 = ((string*)empty_string);
	if(1) {
		string* v1 = ((string*)empty_string);
		string* v2 = ((string*)empty_string);
		
		string_decref(v1);
		string_decref(v2);
		string_decref(v4);
		return 1;
		v1 = string_assign(v1, ((string*)s4));
		v2 = string_assign(v2, ((string*)s4));
		string_decref(v1);
		string_decref(v2);
	} else {
		string* v3 = ((string*)empty_string);
		v3 = string_assign(v3, ((string*)s5));
		string_decref(v3);
	}
	v4 = string_assign(v4, ((string*)s6));
	while(0) {
		string* v5 = ((string*)empty_string);
		v5 = string_assign(v5, ((string*)s7));
		string_decref(v5);
	}
	
	string_decref(v4);
	return 4;
	string_decref(v4);
}
int main(int argc, char **argv) {
	v0 = string_assign(v0, ((string*)s3));
	string_decref(v0);
	debug("num left allocs %i\n", num_mallocs);
}
