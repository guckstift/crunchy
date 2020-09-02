#include <stdio.h>
int i = 0;
int j = 0;
int main(int argc, char **argv) {
 i = 9;
 j = i;
 i = - j;
 j = - j;
 j = - 1;
 if(1) {
  printf("%i\n", i);
 }
}
