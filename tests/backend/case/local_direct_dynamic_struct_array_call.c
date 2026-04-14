struct Pair {
  int x;
  int y;
};

int get_y(struct Pair *p) {
  return p->y;
}

int main(void) {
  struct Pair pairs[2] = {{1, 2}, {3, 4}};
  int i = 1;
  return get_y(&pairs[i]);
}
