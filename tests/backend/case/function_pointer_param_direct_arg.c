typedef int (*Fn)(int);

int inc(int x) { return x + 1; }

int apply(Fn fn, int x) { return fn(x); }

int main(void) { return apply(inc, 4); }
