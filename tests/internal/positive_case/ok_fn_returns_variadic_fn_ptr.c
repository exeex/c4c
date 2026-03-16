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

int main(void) {
  return pickv()(10, 32) == 42 ? 0 : 1;
}
