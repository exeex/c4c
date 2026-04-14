typedef int (*ternary_fn)(int, int, int);

int call_param(ternary_fn f, int x, int y, int z) { return f(x, y, z); }
