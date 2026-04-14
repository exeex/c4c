struct Pair {
  int a;
  int b;
};

struct Pair id_pair(struct Pair p) { return p; }

int main(void) {
  struct Pair p = {4, 9};
  struct Pair q = id_pair(p);
  return q.b;
}
