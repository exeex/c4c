#include <stdarg.h>

int variadic_double_bytes(int seed, ...) {
  va_list ap;
  double second;
  unsigned char *bytes;

  va_start(ap, seed);
  second = va_arg(ap, double);
  va_end(ap);

  bytes = (unsigned char *)&second;
  return seed + bytes[6] + bytes[7];
}

int main(void) {
  return variadic_double_bytes(1, 2.25);
}
