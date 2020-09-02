#include <stdio.h>
int i = 10;
char* name = "guckstift";
char* user = 0;
int main(int argc, char **argv) {
 while((i>0)) {
  printf("%i\n", (i*i));
  i = (i-1);
 }
 user = name;
 printf("Hello\n");
 printf("%s\n", name);
}
