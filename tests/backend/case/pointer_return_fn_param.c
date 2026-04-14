typedef int *(*PtrFn)(int *);

int *id_ptr(int *p) { return p; }

int *use(PtrFn fn, int *p) { return fn(p); }

int main(void) { return 0; }
