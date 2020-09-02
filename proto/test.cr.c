#include <stdio.h>
int i = 0;
int main(int argc, char **argv) {
 while((i<10)) {
  int square = 0;
  i = (i+1);
  square = (i*i);
  printf("%i ^ 2 = %i\n", i, square);
 }
}
