#include <stdarg.h>

typedef struct MixedFloatInt {
  float x;
  int y;
} MixedFloatInt;

int variadic_mixed_float_int_bytes(int seed, ...) {
  va_list ap;
  MixedFloatInt value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedFloatInt);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[7];
}

int main(void) {
  MixedFloatInt value = {2.25f, 0x11223344};
  return variadic_mixed_float_int_bytes(1, value);
}
