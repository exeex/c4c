struct Pair {
  int a;
  int b;
};

struct Pair make_pair(void) {
  struct Pair p = {3, 7};
  return p;
}

int main(void) {
  struct Pair p = make_pair();
  return p.b;
}
