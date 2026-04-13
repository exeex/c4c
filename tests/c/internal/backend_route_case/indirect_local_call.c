typedef int (*unary_fn)(int);

int call_local(unary_fn f, int x) {
  unary_fn g = f;
  return g(x);
}
