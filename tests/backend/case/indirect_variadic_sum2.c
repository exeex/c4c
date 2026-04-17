#include <stdarg.h>

typedef int (*VarFn)(int, ...);

int sum2(int first, ...) {
  va_list ap;
  int second;
  va_start(ap, first);
  second = va_arg(ap, int);
  va_end(ap);
  return first + second;
}

int apply(VarFn fn) {
  return fn(10, 32);
}

int main(void) {
  return apply(sum2);
}
