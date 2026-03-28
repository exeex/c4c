// Compare-mode smoke test: non-control-flow statement dispatcher coverage.

int run(int seed) {
  int x = seed + 1;
  int y = x * 3;

  x + y;
  y = y - 2;
  asm volatile("" : : "g"(x), "g"(y) : "memory");

  return y + x;
}

int main(void) {
  return run(4) == 20 ? 0 : 1;
}
