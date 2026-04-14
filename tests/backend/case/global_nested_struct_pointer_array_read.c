int x = 10;
int y = 21;

struct Inner {
  int a;
  int *p;
};

struct Outer {
  int b;
  struct Inner inner;
};

struct Outer groups[2] = {{1, {2, &x}}, {3, {4, &y}}};

int main(void) {
  return *groups[1].inner.p;
}
