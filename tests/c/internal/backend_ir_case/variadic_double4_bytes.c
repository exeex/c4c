#include <stdarg.h>

typedef struct Double4 {
  double lane0;
  double lane1;
  double lane2;
  double lane3;
} Double4;

int variadic_double4_bytes(int seed, ...) {
  va_list ap;
  Double4 value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, Double4);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[8] + bytes[16] + bytes[24];
}

int main(void) {
  Double4 value = {1.0, 2.25, 3.5, 4.75};
  return variadic_double4_bytes(1, value);
}
