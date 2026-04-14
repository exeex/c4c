typedef int (*ternary_fn)(int, int, int);

int call_local(ternary_fn f, int x, int y, int z) {
  ternary_fn g = f;
  return g(x, y, z);
}
