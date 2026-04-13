int arr[3] = {3, 5, 8};
int *gp = &arr[1];

struct S {
  int a;
  int *p;
};

struct S s = {.p = gp, .a = 1};

int main(void) { return s.p[1]; }
