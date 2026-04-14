typedef int (*i32_ptr_binary_fn)(int, int *);

int call_local(i32_ptr_binary_fn f, int x, int *p) {
  i32_ptr_binary_fn g = f;
  return g(x, p);
}
