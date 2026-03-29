#include <stdarg.h>

int sum2(int first, ...) {
  va_list ap;
  int second;
  va_start(ap, first);
  second = va_arg(ap, int);
  va_end(ap);
  return first + second;
}

int (*pickv(void))(int, ...) {
  return sum2;
}

int sum_mix(int first, ...) {
  va_list ap;
  va_start(ap, first);
  double second = va_arg(ap, double);
  int third = va_arg(ap, int);
  double fourth = va_arg(ap, double);
  va_end(ap);
  return first + (int)second + third + (int)fourth;
}

int (*pick_mix(void))(int, ...) {
  return sum_mix;
}

int main(void) {
  int sum_ok = pickv()(10, 32) == 42;
  int mix = pick_mix()(5, 18.0, 7, 12.0);
  int mix_ok = mix == 42;
  return (sum_ok && mix_ok) ? 0 : 1;
}
