typedef int *(*ptr_ret_binary_fn)(int *, int);

int *call_local(ptr_ret_binary_fn f, int *p, int x) {
  ptr_ret_binary_fn g = f;
  return g(p, x);
}
