typedef int (*unary_fn)(int);

int call_param(unary_fn f, int x) { return f(x); }
