struct Inner {
  char tag;
  int xs[3];
};

struct Outer {
  int prefix;
  struct Inner *inner;
};

int get_at(struct Outer *p, int i) {
  return p->inner->xs[i];
}

int main(void) {
  struct Inner inner = {2, {4, 8, 11}};
  struct Outer outer = {5, &inner};
  return get_at(&outer, 2);
}
