int arr[3] = {3, 5, 8};
int *gp = &arr[1];

struct Inner {
  int *p;
};

struct Outer {
  int a;
  struct Inner inner;
};

struct Outer s = {1, {gp}};

int main(void) { return *s.inner.p; }
