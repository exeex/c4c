// Phase 1: Rvalue reference binding to temporary expressions.

int square(int x) { return x * x; }
int add(int a, int b) { return a + b; }

int main() {
  // Bind to function return value (rvalue)
  int&& r1 = square(5);
  if (r1 != 25) return 1;

  // Bind to nested function call
  int&& r2 = add(square(2), square(3));
  if (r2 != 13) return 2;

  // Bind to cast expression
  int&& r3 = (int)3.14;
  if (r3 != 3) return 3;

  // Modify through rvalue ref
  r1 = 100;
  if (r1 != 100) return 4;

  return 0;
}
