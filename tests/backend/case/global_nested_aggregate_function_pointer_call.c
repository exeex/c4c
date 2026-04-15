int inc(int x) { return x + 1; }

struct Holder {
  int (*fn)(int);
};

struct Outer {
  int tag;
  struct Holder inner;
};

struct Outer g = {7, {inc}};

int main(void) {
  return g.inner.fn(4);
}
