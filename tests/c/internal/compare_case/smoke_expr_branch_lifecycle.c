// Compare-mode smoke test: short-circuit and ternary branch lifecycles.

int bump(int *slot, int delta) {
  *slot += delta;
  return *slot;
}

int run(void) {
  int x = 0;
  int a = 0 && bump(&x, 1);
  int b = 1 || bump(&x, 10);
  int c = 1 && bump(&x, 2);
  int d = 0 || bump(&x, 3);
  int e = (x > 4) ? bump(&x, 4) : bump(&x, 40);
  int f = (x < 0) ? bump(&x, 50) : bump(&x, 5);
  return x + a + b + c + d + e + f;
}

int main(void) {
  return run() == 40 ? 0 : 1;
}
