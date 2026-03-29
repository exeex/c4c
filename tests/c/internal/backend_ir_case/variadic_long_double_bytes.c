#include <stdarg.h>

int variadic_long_double_bytes(int seed, ...) {
  va_list ap;
  long double second;
  unsigned char *bytes;

  va_start(ap, seed);
  second = va_arg(ap, long double);
  va_end(ap);

  bytes = (unsigned char *)&second;
  return seed + bytes[14] + bytes[15];
}

int main(void) {
  return variadic_long_double_bytes(1, 1.0L);
}
