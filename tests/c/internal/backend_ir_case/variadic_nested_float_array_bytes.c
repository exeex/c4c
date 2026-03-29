#include <stdarg.h>

typedef struct NestedFloatArray {
  float lane[2][2];
} NestedFloatArray;

int variadic_nested_float_array_bytes(int seed, ...) {
  va_list ap;
  NestedFloatArray value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, NestedFloatArray);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[4] + bytes[8] + bytes[12];
}

int main(void) {
  NestedFloatArray value = {{{1.0f, 2.25f}, {3.5f, 4.75f}}};
  return variadic_nested_float_array_bytes(1, value);
}
