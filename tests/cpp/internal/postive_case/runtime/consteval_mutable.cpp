// Test: consteval with mutable locals, loops, and assignments

consteval int sum_to(int n) {
  int total = 0;
  for (int i = 1; i <= n; i = i + 1) {
    total = total + i;
  }
  return total;
}

consteval int factorial(int n) {
  int result = 1;
  int i = 2;
  while (i <= n) {
    result = result * i;
    i = i + 1;
  }
  return result;
}

consteval int do_while_test(int n) {
  int x = 0;
  do {
    x = x + n;
    n = n - 1;
  } while (n > 0);
  return x;
}

consteval int with_assign(int x) {
  int a = x;
  a = a + 10;
  int b = a * 2;
  return b;
}

consteval int with_compound(int x) {
  int a = x;
  a = a + 5;
  a = a * 2;
  return a;
}

consteval int with_break(int n) {
  int sum = 0;
  for (int i = 0; i < 100; i = i + 1) {
    if (i >= n)
      break;
    sum = sum + i;
  }
  return sum;
}

consteval int with_continue(int n) {
  int sum = 0;
  for (int i = 0; i < n; i = i + 1) {
    if (i == 3)
      continue;
    sum = sum + i;
  }
  return sum;
}

int main() {
  // For loop
  int a = sum_to(10);
  if (a != 55) return 1;

  // While loop
  int b = factorial(6);
  if (b != 720) return 2;

  // Do-while loop
  int c = do_while_test(4);
  if (c != 10) return 3;

  // Simple assignment
  int d = with_assign(5);
  if (d != 30) return 4;

  // Compound-style reassignment
  int e = with_compound(3);
  if (e != 16) return 5;

  // For with break
  int f = with_break(5);
  if (f != 10) return 6;

  // For with continue
  int g = with_continue(6);
  if (g != 12) return 7;

  return 0;
}
