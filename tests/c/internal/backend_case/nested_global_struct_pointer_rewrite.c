int x = 10;
int y = 21;
int z = 34;
int *gp = &y;
int *gq = &z;

struct Inner {
  int *p;
};

struct Outer {
  int a;
  struct Inner inner;
};

struct Outer s = {1, {&x}};

int main(void) {
  int **pp = &s.inner.p;
  s.inner.p = gp;
  *pp = gq;
  return *s.inner.p;
}
