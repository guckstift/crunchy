#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "H";
double v0 = 0.0;
int v1 = 0;
unsigned char v2 = 0;
string* v3 = ((string*)empty_string);
string* v4 = ((string*)empty_string);
unsigned char v5 = 0;
unsigned char v6 = (9==9);
int main(int argc, char **argv) {
	v0 = 9.1;
	v0 = (v0+((double)8));
	v0 = (((double)1)+v0);
	v0 = (1.2+5.2);
	v0 = (v0+((double)1));
	printf("%s\n", d2s(v0, 0));
	v1 = (1+0);
	v1 = (1-21);
	v1 = (1+1);
	printf("%i\n", v1);
	v0 = (((((1.5+((double)1))+((double)1))+((double)2))-0.3)+((double)0));
	printf("%s\n", d2s(v0, 0));
	printf("%s\n", (((double)1)!=1.3) ? "true" : "false");
	printf("%s\n", (1!=0) ? "true" : "false");
	printf("%s\n", (1>0) ? "true" : "false");
	printf("%s\n", (1<0) ? "true" : "false");
	v3 = string_assign(v3, string_concat(((string*)s3), ((string*)empty_string)));
	v4 = string_assign(v4, string_concat(string_concat(((string*)empty_string), ((string*)s3)), ((string*)empty_string)));
	printf("%s\n", string_equ(((string*)s3), v3) ? "true" : "false");
	v5 = string_equ(((string*)s3), v3);
	string_decref(v3);
	string_decref(v4);
	debug("num left allocs %i\n", num_mallocs);
}
