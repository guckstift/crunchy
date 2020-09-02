#include <stdio.h>
int i = 0;
int main(int argc, char **argv) {
 i = 10;
 while((i>0)) {
  printf("%i\n", i);
  i = (i-1);
 }
}
