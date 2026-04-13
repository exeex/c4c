struct Inner {
  int xs[3];
};

struct Outer {
  struct Inner inner;
};

struct Outer groups[2] = {{{1, 2, 3}}, {{4, 5, 6}}};

int main(void) {
  int *p = &groups[0].inner.xs[0];
  p[5] = 9;
  return groups[1].inner.xs[2];
}
