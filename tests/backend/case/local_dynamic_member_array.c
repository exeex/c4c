struct Pair {
  int xs[3];
};

int get_at(struct Pair *p, int i) {
  return p->xs[i];
}

int main(void) {
  struct Pair p = {{4, 8, 11}};
  return get_at(&p, 2);
}
