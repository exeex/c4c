#include <stdarg.h>

typedef struct MixedDoubleShort {
  double x;
  short y;
} MixedDoubleShort;

int variadic_mixed_double_short_bytes(int seed, ...) {
  va_list ap;
  MixedDoubleShort value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedDoubleShort);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[15];
}

int main(void) {
  MixedDoubleShort value = {2.25, 0x3344};
  return variadic_mixed_double_short_bytes(1, value);
}
