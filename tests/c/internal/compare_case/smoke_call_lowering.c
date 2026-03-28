// Compare-mode smoke test: direct, indirect, and variadic call lowering.
extern int printf(const char *, ...);

typedef int (*combine_fn)(short, short);

int add_pair(short lhs, short rhs) { return lhs + rhs; }
int sub_pair(short lhs, short rhs) { return lhs - rhs; }
void bump_total(int *slot) { *slot += 4; }
int sum_window(const int *values, short delta) {
  return values[0] + values[1] + delta;
}

int call_direct(int lhs, int rhs) {
  return add_pair(lhs, rhs);
}

int call_indirect(combine_fn fn, int lhs, int rhs) {
  return fn(lhs, rhs);
}

int call_decay(short base) {
  int values[2] = {base, base + 3};
  return sum_window(values, base);
}

int call_void_direct(int seed) {
  int total = seed;
  bump_total(&total);
  return total;
}

int call_variadic(int value, float scale) {
  return printf("value=%d scale=%.1f\n", value, scale);
}

int main(void) {
  const int direct = call_direct(9, 4);
  combine_fn fn = sub_pair;
  const int indirect = call_indirect(fn, 9, 4);
  const int decay = call_decay(6);
  const int void_direct = call_void_direct(decay);
  call_variadic(direct + indirect + void_direct, 1.5f);
  return (direct == 13 && indirect == 5 && decay == 21 && void_direct == 25) ? 0 : 1;
}
