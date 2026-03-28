#include <stdarg.h>

typedef struct MixedShortDouble {
  short x;
  double y;
} MixedShortDouble;

int variadic_mixed_short_double_bytes(int seed, ...) {
  va_list ap;
  MixedShortDouble value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedShortDouble);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[15];
}

int main(void) {
  MixedShortDouble value = {0x3344, 2.25};
  return variadic_mixed_short_double_bytes(1, value);
}
