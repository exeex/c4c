typedef int (*ptr_binary_fn)(int *, int);

int call_param(ptr_binary_fn f, int *p, int x) { return f(p, x); }
