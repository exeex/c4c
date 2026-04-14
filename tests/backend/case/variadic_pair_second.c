#include <stdarg.h>

typedef struct Pair {
  int x;
  int y;
} Pair;

int variadic_pair_second(int seed, ...) {
  va_list ap;
  Pair second;
  va_start(ap, seed);
  second = va_arg(ap, Pair);
  va_end(ap);
  return seed + second.y;
}

int main(void) {
  Pair pair = {3, 8};
  return variadic_pair_second(1, pair);
}
