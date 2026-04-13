typedef int (*quaternary_fn)(int, int, int, int);

int call_param(quaternary_fn f, int w, int x, int y, int z) { return f(w, x, y, z); }
