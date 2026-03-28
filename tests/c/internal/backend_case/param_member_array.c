struct Pair {
  int xs[2];
};

int get_second(struct Pair p) { return p.xs[1]; }

int main() {
  struct Pair p;
  p.xs[0] = 4;
  p.xs[1] = 6;
  return get_second(p);
}
