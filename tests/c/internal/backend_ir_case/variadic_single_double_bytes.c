#include <stdarg.h>

typedef struct SingleDouble {
  double lane0;
} SingleDouble;

int variadic_single_double_bytes(int seed, ...) {
  va_list ap;
  SingleDouble value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, SingleDouble);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[7];
}

int main(void) {
  SingleDouble value = {2.25};
  return variadic_single_double_bytes(1, value);
}
