#include <stdarg.h>

typedef unsigned char u8;

struct Bytes9 {
  u8 x[9];
};

struct Bytes9 a = {"abcdefghi"};
struct Bytes9 b = {"bcdefghij"};
struct Bytes9 c = {"cdefghijk"};
struct Bytes9 d = {"defghijkl"};
struct Bytes9 e = {"efghijklm"};
struct Bytes9 f = {"fghijklmn"};
struct Bytes9 g = {"ghijklmno"};
struct Bytes9 h = {"hijklmnop"};
struct Bytes9 i = {"ijklmnopq"};

int pick_ninth(int seed, ...) {
  va_list ap;
  struct Bytes9 current;
  int result;

  va_start(ap, seed);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  current = va_arg(ap, struct Bytes9);
  result = current.x[0] + current.x[4] + current.x[8] + seed;
  va_end(ap);

  return result;
}

int main(void) {
  return pick_ninth(1, a, b, c, d, e, f, g, h, i) != 322;
}
