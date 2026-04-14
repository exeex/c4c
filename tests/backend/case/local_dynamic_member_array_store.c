struct Pair {
  int xs[3];
};

void set_at(struct Pair *p, int i, int v) {
  p->xs[i] = v;
}

int main(void) {
  struct Pair p = {{4, 8, 11}};
  set_at(&p, 1, 13);
  return p.xs[1];
}
