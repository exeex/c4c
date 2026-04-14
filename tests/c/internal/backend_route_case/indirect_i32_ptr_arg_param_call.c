typedef int (*i32_ptr_binary_fn)(int, int *);

int call_param(i32_ptr_binary_fn f, int x, int *p) { return f(x, p); }
