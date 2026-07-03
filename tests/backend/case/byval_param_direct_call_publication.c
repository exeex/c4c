struct Trio {
  int a;
  int b;
  int c;
};

int consume_trio(int seed, struct Trio value) {
  return seed + value.a + value.b + value.c;
}

int forward_trio(struct Trio value, int seed) {
  return consume_trio(seed, value);
}

int main(void) {
  struct Trio value = {1, 2, 3};
  return forward_trio(value, 4);
}
