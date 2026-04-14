struct Inner {
  int tag;
  int xs[3];
};

struct Outer {
  int a;
  struct Inner inner;
};

struct Outer s = {1, {2, {3, 5, 8}}};
int *gp = &s.inner.xs[1];

int main(void) { return *gp; }
