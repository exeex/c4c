struct FnHolder {
  int (*fn)(int);
};

int inc(int x) { return x + 1; }

int apply(struct FnHolder *h, int x) {
  return h->fn(x);
}

int main(void) {
  struct FnHolder h = {inc};
  return apply(&h, 4);
}
