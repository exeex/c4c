typedef int (*i32_ptr_ptr_ternary_fn)(int, int *, int *);

int call_local(i32_ptr_ptr_ternary_fn f, int x, int *p, int *q) {
  i32_ptr_ptr_ternary_fn g = f;
  return g(x, p, q);
}
