int x = 10;
int y = 21;
int *gp = &y;

struct Inner {
  int a;
  int *p;
};

struct Outer {
  int b;
  struct Inner inner;
};

struct Outer groups[2] = {{1, {2, &x}}, {3, {4, &x}}};

int main(void) {
  int **pp = &groups[1].inner.p;
  *pp = gp;
  return **pp;
}
