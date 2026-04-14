typedef int (*unary_fn)(int);

int call_select(int flag, unary_fn f, unary_fn g, int x) {
  return (flag ? f : g)(x);
}
