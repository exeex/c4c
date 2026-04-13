struct Inner {
  int xs[3];
};

struct Outer {
  struct Inner inner;
};

struct Outer s = {{{1, 2, 3}}};

int main(void) {
  return s.inner.xs[2];
}
