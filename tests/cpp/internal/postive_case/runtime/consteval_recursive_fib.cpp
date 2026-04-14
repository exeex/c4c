// Test: recursive consteval calls should evaluate through nested immediate calls.

consteval int fib(int n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

consteval int fib_plus_one(int n) {
  return fib(n) + 1;
}

int main() {
  int a = fib(0);
  if (a != 0) return 1;

  int b = fib(1);
  if (b != 1) return 2;

  int c = fib(7);
  if (c != 13) return 3;

  int d = fib_plus_one(8);
  if (d != 22) return 4;

  return 0;
}
