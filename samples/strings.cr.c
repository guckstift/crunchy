#include "c_lib.h"
char empty_string[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00" "";
char false_string[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "false";
char true_string[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "true";
char s3[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "hello";
char s4[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "world";
char s5[] = "\xFF\xFF\xFF\xFF\x02\x00\x00\x00" "aw";
char s6[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Hello";
char s7[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" " World";
char s8[] = "\xFF\xFF\xFF\xFF\x01\x00\x00\x00" "!";
char s9[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "jkjd";
char s10[] = "\xFF\xFF\xFF\xFF\x08\x00\x00\x00" "  asdfs ";
char s11[] = "\xFF\xFF\xFF\xFF\x05\x00\x00\x00" "Danny";
char s12[] = "\xFF\xFF\xFF\xFF\x06\x00\x00\x00" "Hello ";
char s13[] = "\xFF\xFF\xFF\xFF\x04\x00\x00\x00" "Bye ";
char s14[] = "\xFF\xFF\xFF\xFF\x02\x00\x00\x00" "!?";
string* v0 = ((string*)empty_string);
string* v1 = ((string*)empty_string);
string* v2 = ((string*)empty_string);
string* v3 = ((string*)empty_string);
int v4 = 9;
string* v5 = ((string*)empty_string);
cr_bool v6 = 1;
int main(int argc, char **argv) {
	v0 = ref_assign(v0, ((string*)s3));
	v1 = ref_assign(v1, ((string*)s4));
	v2 = ref_assign(v2, ((string*)s5));
	v3 = ref_assign(v3, string_concats(3, v0, v1, v2));
	{string* c0 = ref_incref(string_concats(3, ((string*)s6), ((string*)s7), ((string*)s8)));
	string* c1 = ref_incref(string_concat(((string*)s9), ((string*)s10)));
	printf("%.*s %i %.*s\n", c0->length, c0->data, v4, c1->length, c1->data);
	ref_decref(c0);
	ref_decref(c1);}
	v5 = ref_assign(v5, ((string*)s11));
	if(v6) {
		{string* c0 = ref_incref(string_concats(3, ((string*)s12), v5, ((string*)s8)));
		printf("%.*s\n", c0->length, c0->data);
		ref_decref(c0);}
	} else {
		{string* c0 = ref_incref(string_concats(3, ((string*)s13), v5, ((string*)s14)));
		printf("%.*s\n", c0->length, c0->data);
		ref_decref(c0);}
	}
	ref_decref(v0);
	ref_decref(v1);
	ref_decref(v2);
	ref_decref(v3);
	ref_decref(v5);
	debug("num left allocs %i\n", num_mallocs);
}
