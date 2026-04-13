typedef int (*binary_fn)(int, int);

int call_param(binary_fn f, int x, int y) { return f(x, y); }
