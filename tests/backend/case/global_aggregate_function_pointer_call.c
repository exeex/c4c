int inc(int x) { return x + 1; }

struct Holder {
  int (*fn)(int);
};

struct Holder g = {inc};

int main(void) {
  return g.fn(4);
}
