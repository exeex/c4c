#include <stdarg.h>

typedef struct Float4 {
  float lane0;
  float lane1;
  float lane2;
  float lane3;
} Float4;

int variadic_float4_bytes(int seed, ...) {
  va_list ap;
  Float4 value;
  unsigned char *bytes;

  va_start(ap, seed);
  value = va_arg(ap, Float4);
  va_end(ap);

  bytes = (unsigned char *)&value;
  return seed + bytes[0] + bytes[4] + bytes[8] + bytes[12];
}

int main(void) {
  Float4 value = {1.0f, 2.25f, 3.5f, 4.75f};
  return variadic_float4_bytes(1, value);
}
