struct Pair {
  int a;
  int b;
};

typedef struct Pair (*PairFn)(struct Pair);

struct Pair id_pair(struct Pair p) { return p; }

int use(PairFn fn, struct Pair p) {
  struct Pair q = fn(p);
  return q.b;
}

int main(void) {
  struct Pair p = {4, 9};
  return use(id_pair, p);
}
