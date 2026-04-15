int inc(int x) { return x + 1; }
int dec(int x) { return x - 1; }

struct Holder {
  int (*fn)(int);
};

struct Outer {
  int tag;
  struct Holder inner[2];
};

struct Outer g = {7, {{dec}, {inc}}};

int main(void) {
  return g.inner[1].fn(4);
}
