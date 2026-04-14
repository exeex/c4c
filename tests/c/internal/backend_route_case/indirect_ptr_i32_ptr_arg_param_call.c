typedef int (*ptr_i32_ptr_ternary_fn)(int *, int, int *);

int call_param(ptr_i32_ptr_ternary_fn f, int *p, int x, int *q) {
  return f(p, x, q);
}
