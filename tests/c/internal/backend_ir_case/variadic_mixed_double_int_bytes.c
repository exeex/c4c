#include <stdarg.h>

typedef struct MixedDoubleInt {
  double x;
  int y;
} MixedDoubleInt;

int variadic_mixed_double_int_bytes(int seed, ...) {
  va_list ap;
  MixedDoubleInt value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedDoubleInt);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[15];
}

int main(void) {
  MixedDoubleInt value = {2.25, 0x11223344};
  return variadic_mixed_double_int_bytes(1, value);
}
