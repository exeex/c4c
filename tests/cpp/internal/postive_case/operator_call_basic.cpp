// Runtime test: operator() (function call operator) on struct types.
struct Adder {
  int base;

  int operator()(int x) {
    return base + x;
  }
};

struct Multiplier {
  int factor;

  int operator()(int a, int b) {
    return factor * (a + b);
  }
};

// Zero-arg operator()
struct Counter {
  int count;

  int operator()() {
    count = count + 1;
    return count;
  }
};

int main() {
  Adder add;
  add.base = 10;
  if (add(5) != 15) return 1;
  if (add(0) != 10) return 2;
  if (add(-3) != 7) return 3;

  Multiplier mul;
  mul.factor = 2;
  if (mul(3, 4) != 14) return 4;
  if (mul(0, 0) != 0) return 5;

  Counter c;
  c.count = 0;
  if (c() != 1) return 6;
  if (c() != 2) return 7;
  if (c() != 3) return 8;

  return 0;
}
