#include <stdlib.h>

extern int printf(const char *, ...);

typedef int (*unary_fn)(int);

static int add_one(int value) {
  return value + 1;
}

static int call_direct(int value) {
  return add_one(value);
}

static int call_indirect(unary_fn fn, int value) {
  return fn(value);
}

int main(void) {
  int total = 0;
  total += call_direct(2);
  total += call_indirect(add_one, 3);
  total += abs(-5);
  total += printf("%s", "z");
  return total == 13 ? 0 : 1;
}
