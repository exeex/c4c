int arr[4] = {3, 5, 8, 13};
int *gp = &arr[1];
int *gpp = gp + 1;

struct Inner {
  int *p;
};

struct Outer {
  int a;
  struct Inner inner;
};

struct Outer s = {1, {gpp}};

int main(void) { return s.inner.p[1]; }
