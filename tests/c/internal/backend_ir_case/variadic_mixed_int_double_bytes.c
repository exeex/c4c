#include <stdarg.h>

typedef struct MixedIntDouble {
  int x;
  double y;
} MixedIntDouble;

int variadic_mixed_int_double_bytes(int seed, ...) {
  va_list ap;
  MixedIntDouble value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedIntDouble);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[15];
}

int main(void) {
  MixedIntDouble value = {0x11223344, 2.25};
  return variadic_mixed_int_double_bytes(1, value);
}
