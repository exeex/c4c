typedef int (*quinary_fn)(int, int, int, int, int);

int call_param(quinary_fn f, int v, int w, int x, int y, int z) { return f(v, w, x, y, z); }
