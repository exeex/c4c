typedef int *(*ptr_ret_binary_fn)(int *, int);

int *call_param(ptr_ret_binary_fn f, int *p, int x) { return f(p, x); }
