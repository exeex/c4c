#include <stdlib.h>

long take_abs_long(long x) {
  return labs(x);
}

int main(void) {
  return (int)take_abs_long(-9L);
}
