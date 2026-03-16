// Test: consteval diagnostics — verify that valid consteval calls still fold
// and produce correct results after adding structured failure diagnostics.

consteval int triple(int x) {
  return x * 3;
}

consteval int abs_val(int x) {
  if (x < 0)
    return -x;
  return x;
}

consteval int sum_range(int lo, int hi) {
  int s = 0;
  for (int i = lo; i <= hi; i = i + 1)
    s = s + i;
  return s;
}

int main() {
  // Basic consteval call
  int a = triple(7);
  if (a != 21) return 1;

  // Consteval with branching
  int b = abs_val(-5);
  if (b != 5) return 2;

  int c = abs_val(3);
  if (c != 3) return 3;

  // Consteval with loop
  int d = sum_range(1, 5);
  if (d != 15) return 4;

  return 0;
}
