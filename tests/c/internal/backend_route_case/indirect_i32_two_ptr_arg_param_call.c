typedef int (*i32_ptr_ptr_ternary_fn)(int, int *, int *);

int call_param(i32_ptr_ptr_ternary_fn f, int x, int *p, int *q) {
  return f(x, p, q);
}
