int arr[4] = {3, 5, 8, 13};
int *gp = &arr[1];
int *gpp = gp + 1;

struct S {
  int a;
  int *p;
};

struct S s = {.p = gpp, .a = 1};

int main(void) { return s.p[1]; }
