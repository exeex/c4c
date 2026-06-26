#include <stdarg.h>

typedef unsigned char u8;

struct Bytes9 {
  u8 x[9];
};

int pick_second_byte(int seed, ...) {
  va_list ap;
  struct Bytes9 first;
  struct Bytes9 second;

  va_start(ap, seed);
  first = va_arg(ap, struct Bytes9);
  second = va_arg(ap, struct Bytes9);
  va_end(ap);

  return first.x[0] + second.x[8] + seed;
}
