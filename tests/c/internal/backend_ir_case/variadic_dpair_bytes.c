#include <stdarg.h>

typedef struct DPair {
  double x;
  double y;
} DPair;

int variadic_dpair_bytes(int seed, ...) {
  va_list ap;
  DPair value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, DPair);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[8] + bytes[15];
}

int main(void) {
  DPair value = {1.0, 2.25};
  return variadic_dpair_bytes(1, value);
}
