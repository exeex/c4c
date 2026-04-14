typedef int (*ptr_ptr_i32_ternary_fn)(int *, int *, int);

int call_local(ptr_ptr_i32_ternary_fn f, int *p, int *q, int x) {
  ptr_ptr_i32_ternary_fn g = f;
  return g(p, q, x);
}
