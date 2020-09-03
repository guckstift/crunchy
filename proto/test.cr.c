#include <stdio.h>
int v0 = 0;
int main(int argc, char **argv) {
	while((v0!=10)) {
		int v1 = 0;
		v0 = (v0+1);
		v1 = (v0*v0);
		printf("%i ^ 2 = %i\n", v0, v1);
	}
	if(1) {
		unsigned char v2 = 1;
		printf("%s\n", v2 ? "true" : "false");
	} else {
		printf("JO\n");
	}
}
