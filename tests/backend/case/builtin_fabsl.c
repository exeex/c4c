#include <math.h>

long double take_fabsl(long double x) {
  return __builtin_fabsl(x);
}

int main(void) {
  return 0;
}
