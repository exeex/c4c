typedef int *(*ptr_unary_ret_fn)(int *);

int *call_local(ptr_unary_ret_fn f, int *p) {
  ptr_unary_ret_fn g = f;
  return g(p);
}
