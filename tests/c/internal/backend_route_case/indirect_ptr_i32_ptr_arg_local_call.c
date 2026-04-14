typedef int (*ptr_i32_ptr_ternary_fn)(int *, int, int *);

int call_local(ptr_i32_ptr_ternary_fn f, int *p, int x, int *q) {
  ptr_i32_ptr_ternary_fn g = f;
  return g(p, x, q);
}
