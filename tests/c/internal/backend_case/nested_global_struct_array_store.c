struct Inner {
  int xs[3];
};

struct Outer {
  struct Inner inner;
};

struct Outer s = {{{1, 2, 3}}};

int main(void) {
  s.inner.xs[1] = 9;
  return s.inner.xs[1];
}
