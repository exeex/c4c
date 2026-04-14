#include <stdarg.h>

int sum2(int first, ...) {
  va_list ap;
  int second;
  va_start(ap, first);
  second = va_arg(ap, int);
  va_end(ap);
  return first + second;
}

int main(void) {
  return sum2(10, 32);
}
