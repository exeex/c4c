struct S {
  int a;
  int xs[3];
};

struct S s = {1, {3, 5, 8}};
int *gp = &s.xs[1];

int main(void) { return *gp; }
