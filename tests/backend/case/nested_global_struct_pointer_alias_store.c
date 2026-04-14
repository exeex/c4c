int x = 10;
int y = 21;
int *gp = &y;

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
  *pp = gp;
  return **pp;
}
