typedef int (*unary_fn)(int);

int inc(int x) { return x + 1; }

unary_fn gp = inc;

int call_global(int x) { return gp(x); }
