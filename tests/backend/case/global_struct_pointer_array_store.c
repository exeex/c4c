int x = 10;
int y = 21;
int *gp = &y;

struct Pair {
  int a;
  int *p;
};

struct Pair pairs[2] = {{1, &x}, {2, &x}};

int main(void) {
  pairs[1].p = gp;
  return *pairs[1].p;
}
