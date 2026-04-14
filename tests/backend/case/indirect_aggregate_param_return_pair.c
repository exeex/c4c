struct Pair {
  int a;
  int b;
};

typedef struct Pair (*PairFn)(struct Pair);

struct Pair id_pair(struct Pair p) { return p; }

int main(void) {
  PairFn fn = id_pair;
  struct Pair p = {4, 9};
  struct Pair q = fn(p);
  return q.b;
}
