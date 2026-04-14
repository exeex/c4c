typedef int (*unary_fn)(int);

int call_select_local_override(int a, int b, unary_fn f, unary_fn g, unary_fn h, int x) {
  unary_fn callee = a ? f : g;
  if (b) {
    callee = h;
  }
  return callee(x);
}
