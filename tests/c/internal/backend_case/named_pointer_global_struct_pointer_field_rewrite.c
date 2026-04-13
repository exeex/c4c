int x = 10;
int y = 21;
int z = 34;
int *gp = &y;
int *gq = &z;

struct S {
  int a;
  int *p;
};

struct S s = {.p = &x, .a = 1};

int main(void) {
  int **pp = &s.p;
  s.p = gp;
  *pp = gq;
  return *s.p;
}
