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
  int g = (x > 10) ? (1 && bump(&x, 6)) : (0 || bump(&x, 60));
  int h = (x < 0) ? (0 && bump(&x, 70)) : (0 || bump(&x, 7));
  int i = (x > 20) ? ((x > 0) ? bump(&x, 8) : bump(&x, 80))
                   : ((x < 0) ? bump(&x, 800) : bump(&x, 9));
  int j = (x < 0) ? (0 && bump(&x, 90))
                  : ((x > 30) ? (1 && bump(&x, 10)) : (0 || bump(&x, 100)));
  int k = (x > 40) ? ((x < 0) ? bump(&x, 110) : (1 && bump(&x, 11)))
                   : ((x > 0) ? (0 || bump(&x, 12)) : bump(&x, 120));
  return x + a + b + c + d + e + f + g + h + i + j + k;
}

int main(void) {
  return run() == 121 ? 0 : 1;
}
