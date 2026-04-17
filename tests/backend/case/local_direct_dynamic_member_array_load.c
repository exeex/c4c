struct Pair {
  int xs[3];
};

int main(void) {
  struct Pair p = {{4, 8, 11}};
  int i = 1;
  return p.xs[i];
}
