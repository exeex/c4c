#include <stdarg.h>

typedef struct MixedCharDouble {
  char x;
  double y;
} MixedCharDouble;

int variadic_mixed_char_double_bytes(int seed, ...) {
  va_list ap;
  MixedCharDouble value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, MixedCharDouble);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[15];
}

int main(void) {
  MixedCharDouble value = {0x44, 2.25};
  return variadic_mixed_char_double_bytes(1, value);
}
