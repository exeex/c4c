typedef int (*ptr_ptr_i32_ternary_fn)(int *, int *, int);

int call_param(ptr_ptr_i32_ternary_fn f, int *p, int *q, int x) {
  return f(p, q, x);
}
