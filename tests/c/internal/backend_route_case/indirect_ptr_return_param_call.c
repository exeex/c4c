typedef int *(*ptr_unary_ret_fn)(int *);

int *call_param(ptr_unary_ret_fn f, int *p) { return f(p); }
