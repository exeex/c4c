struct Pair {
  int x;
  int y;
};

int main(void) {
  struct Pair p = {1, 2};
  p.y = 5;
  return p.x + p.y;
}
