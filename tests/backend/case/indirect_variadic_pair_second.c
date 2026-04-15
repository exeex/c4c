#include <stdarg.h>

typedef struct Pair {
  int x;
  int y;
} Pair;

typedef int (*VarPairFn)(int, ...);

int variadic_pair_second(int seed, ...) {
  va_list ap;
  Pair second;
  va_start(ap, seed);
  second = va_arg(ap, Pair);
  va_end(ap);
  return seed + second.y;
}

int apply(VarPairFn fn, Pair pair) {
  return fn(1, pair);
}

int main(void) {
  Pair pair = {3, 8};
  return apply(variadic_pair_second, pair);
}
