int x = 10;

struct Inner {
  int *p;
};

struct Outer {
  int a;
  struct Inner inner;
};

struct Outer s = {1, {&x}};

int main(void) { return *s.inner.p; }
