struct Inner {
  int xs[3];
};

struct Outer {
  struct Inner inner;
};

struct Outer s = {{{1, 2, 3}}};

int main(void) {
  int *p = &s.inner.xs[0];
  p[2] = 9;
  return s.inner.xs[2];
}
