typedef int (*ptr_ptr_binary_fn)(int *, int *);

int call_param(ptr_ptr_binary_fn f, int *p, int *q) { return f(p, q); }
