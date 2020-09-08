#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "*";
char s4[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Nice!";
char s5[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" " Danny";
void v2() {
	string* v0 = ((string*)empty_string);
	int v1 = 10;
	v0 = string_assign(v0, ((string*)empty_string));
	while((v1>0)) {
		printf("%i\n", v1);
		v1 = (v1-1);
		v0 = string_assign(v0, string_concat(v0, ((string*)s3)));
		printf("%.*s\n", v0->length, v0->data);
	}
	
	string_decref(v0);
	return;
	string_decref(v0);
}
int v3() {
	
	return 0;
}
int main(int argc, char **argv) {
	v2();
	{string* c0 = string_incref(string_concat(((string*)s4), ((string*)s5)));
	printf("%.*s\n", c0->length, c0->data);
	string_decref(c0);}
	v2();
	debug("num left allocs %i\n", num_mallocs);
}
