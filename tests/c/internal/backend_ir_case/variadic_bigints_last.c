#include <stdarg.h>

typedef struct BigInts {
  int a;
  int b;
  int c;
  int d;
  int e;
} BigInts;

int variadic_bigints_last(int seed, ...) {
  va_list ap;
  BigInts value;

  va_start(ap, seed);
  value = va_arg(ap, BigInts);
  va_end(ap);

  return seed + value.e;
}

int main(void) {
  BigInts value = {1, 2, 3, 4, 9};
  return variadic_bigints_last(1, value);
}
