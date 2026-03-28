// Compare-mode smoke test: AArch64 va_arg register/stack join lifecycles.

#include <stdarg.h>

long sum_gp_args(int marker, ...) {
  va_list ap;
  long sum = marker;
  va_start(ap, marker);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  sum += va_arg(ap, int);
  va_end(ap);
  return sum;
}

double sum_fp_args(int marker, ...) {
  va_list ap;
  double sum = (double)marker;
  va_start(ap, marker);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  sum += va_arg(ap, double);
  va_end(ap);
  return sum;
}

int main(void) {
  long gp = sum_gp_args(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  double fp = sum_fp_args(0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
  return (gp == 45 && fp == 45.0) ? 0 : 1;
}
