#include <stdarg.h>

typedef struct FloatArray2 {
  float lane[2];
} FloatArray2;

int variadic_float_array_bytes(int seed, ...) {
  va_list ap;
  FloatArray2 value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, FloatArray2);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[4];
}

int main(void) {
  FloatArray2 value = {{1.0f, 2.25f}};
  return variadic_float_array_bytes(1, value);
}
