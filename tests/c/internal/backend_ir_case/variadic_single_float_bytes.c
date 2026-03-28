#include <stdarg.h>

typedef struct SingleFloat {
  float lane0;
} SingleFloat;

int variadic_single_float_bytes(int seed, ...) {
  va_list ap;
  SingleFloat value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, SingleFloat);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[3];
}

int main(void) {
  SingleFloat value = {2.25f};
  return variadic_single_float_bytes(1, value);
}
