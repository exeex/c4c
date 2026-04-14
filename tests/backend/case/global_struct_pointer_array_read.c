int x = 10;
int y = 21;

struct Pair {
  int a;
  int *p;
};

struct Pair pairs[2] = {{1, &x}, {2, &y}};

int main(void) {
  return *pairs[1].p;
}
