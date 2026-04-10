#include <stdarg.h>

typedef struct Pair {
  int x;
  int y;
} Pair;

static int take_pair(Pair pair) {
  return pair.x * 10 + pair.y;
}

static int mix_call_args(int seed, Pair fixed, ...) {
  va_list ap;
  va_start(ap, fixed);
  int promoted_int = va_arg(ap, int);
  double promoted_double = va_arg(ap, double);
  Pair tail = va_arg(ap, Pair);
  va_end(ap);
  return seed + take_pair(fixed) + promoted_int + (int)promoted_double + tail.x + tail.y;
}

int main(void) {
  Pair fixed = {2, 3};
  Pair tail = {4, 5};
  signed char small = 6;
  float fp = 7.0f;
  return mix_call_args(1, fixed, small, fp, tail) == 46 ? 0 : 1;
}
