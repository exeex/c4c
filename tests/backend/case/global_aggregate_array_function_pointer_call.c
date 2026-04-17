int inc(int x) { return x + 1; }
int dec(int x) { return x - 1; }

struct Holder {
  int (*fn)(int);
};

struct Holder gs[2] = {{dec}, {inc}};

int main(void) {
  return gs[1].fn(4);
}
