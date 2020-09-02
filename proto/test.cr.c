#include <stdio.h>
int foo = 1;
int baz = 0;
int x = 0;
int y = 0;
int main(int argc, char **argv) {
 foo = 9;
 baz = foo;
 baz = foo;
 x = (foo+baz);
 y = foo;
}
