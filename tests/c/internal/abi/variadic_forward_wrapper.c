#include <stdarg.h>

struct pair {
  long lhs;
  long rhs;
};

static double reduce_with_format(const char *fmt, va_list ap) {
  double total = 0.0;
  for (const char *p = fmt; *p; ++p) {
    if (*p == 'i') {
      total += va_arg(ap, int);
    } else if (*p == 'd') {
      total += va_arg(ap, double);
    } else if (*p == 'p') {
      struct pair item = va_arg(ap, struct pair);
      total += (double)(item.lhs + item.rhs);
    }
  }
  return total;
}

static double forward_reduce(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  double sum = reduce_with_format(fmt, ap);
  va_end(ap);
  return sum;
}

int main(void) {
  const struct pair pair_value = {7, 9};
  double total = forward_reduce("idpi", 4, 5.5, pair_value, 6);
  return (total == 31.5) ? 0 : 1;
}
