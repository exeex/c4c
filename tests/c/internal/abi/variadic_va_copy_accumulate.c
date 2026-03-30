#include <stdarg.h>

static long sum_pairs(int count, va_list ap) {
  long total = 0;
  for (int i = 0; i < count; ++i) {
    total += va_arg(ap, long);
    double d = va_arg(ap, double);
    if (d > 0.0) {
      total += (long)(d + 0.0001);
    }
  }
  return total;
}

static long sum_pairs_twice(int count, ...) {
  va_list ap;
  va_start(ap, count);
  va_list copy;
  va_copy(copy, ap);
  long first = sum_pairs(count, ap);
  long second = sum_pairs(count, copy);
  va_end(copy);
  va_end(ap);
  return first + second;
}

int main(void) {
  long expect_once = 10 + 20 + 30 + 40;
  long expect_twice = expect_once * 2;
  long value = sum_pairs_twice(2, 10L, 20.0, 30L, 40.0);
  return value == expect_twice ? 0 : 1;
}
