int x = 10;
int y = 21;
int *gp = &y;

struct S {
  int a;
  int *p;
};

struct S s = {.p = &x, .a = 1};

int main(void) {
  int **pp = &s.p;
  *pp = gp;
  return **pp;
}
