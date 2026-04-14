int x = 1;
int arr[3] = {3, 5, 8};
int *gp = &arr[1];
int *gpp = gp + 1;

struct Pair {
  int a;
  int *p;
};

struct Pair pairs[2] = {{1, &x}, {2, gpp}};

int main(void) { return *pairs[1].p; }
