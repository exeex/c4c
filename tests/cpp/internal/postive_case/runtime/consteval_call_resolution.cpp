// Test: consteval call resolution — verify that consteval calls are resolved
// at compile time and never fall through to runtime call lowering.

consteval int factorial(int n) {
  int result = 1;
  for (int i = 2; i <= n; i = i + 1)
    result = result * i;
  return result;
}

consteval int fib(int n) {
  if (n <= 1) return n;
  int a = 0;
  int b = 1;
  for (int i = 2; i <= n; i = i + 1) {
    int t = a + b;
    a = b;
    b = t;
  }
  return b;
}

consteval int choose(int n, int k) {
  return factorial(n) / (factorial(k) * factorial(n - k));
}

int main() {
  // Direct consteval call
  int a = factorial(5);
  if (a != 120) return 1;

  // Consteval calling consteval (chained resolution)
  int b = choose(5, 2);
  if (b != 10) return 2;

  // Consteval result used in arithmetic
  int c = factorial(3) + factorial(4);
  if (c != 30) return 3;

  // Consteval with iterative algorithm
  int d = fib(10);
  if (d != 55) return 4;

  return 0;
}
