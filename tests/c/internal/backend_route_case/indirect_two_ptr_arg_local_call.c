typedef int (*ptr_ptr_binary_fn)(int *, int *);

int call_local(ptr_ptr_binary_fn f, int *p, int *q) {
  ptr_ptr_binary_fn g = f;
  return g(p, q);
}
