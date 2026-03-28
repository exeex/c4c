// Compare-mode smoke test: direct, indirect, and variadic call lowering.
extern int printf(const char *, ...);

typedef int (*combine_fn)(short, short);

int add_pair(short lhs, short rhs) { return lhs + rhs; }
int sub_pair(short lhs, short rhs) { return lhs - rhs; }

int call_direct(int lhs, int rhs) {
  return add_pair(lhs, rhs);
}

int call_indirect(combine_fn fn, int lhs, int rhs) {
  return fn(lhs, rhs);
}

int call_variadic(int value, float scale) {
  return printf("value=%d scale=%.1f\n", value, scale);
}

int main(void) {
  const int direct = call_direct(9, 4);
  combine_fn fn = sub_pair;
  const int indirect = call_indirect(fn, 9, 4);
  call_variadic(direct + indirect, 1.5f);
  return (direct == 13 && indirect == 5) ? 0 : 1;
}
