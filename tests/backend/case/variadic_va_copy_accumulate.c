#include <stdarg.h>

long variadic_va_copy_accumulate(int seed, ...) {
  va_list ap;
  va_start(ap, seed);
  va_list copy;
  va_copy(copy, ap);
  long first = va_arg(ap, long);
  long second = va_arg(copy, long);
  va_end(copy);
  va_end(ap);
  return seed + first + second;
}

int main(void) {
  return variadic_va_copy_accumulate(1, 10L) != 21L;
}
