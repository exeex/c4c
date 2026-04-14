// Phase 1: Rvalue reference binding to literals and constant expressions.

int main() {
  // Bind to integer literal
  int&& r1 = 10;
  if (r1 != 10) return 1;

  // Bind to long literal
  long&& r2 = 999L;
  if (r2 != 999) return 2;

  // Bind to char literal
  char&& r3 = 'A';
  if (r3 != 65) return 3;

  // Bind to arithmetic expression (rvalue)
  int&& r4 = 3 + 4;
  if (r4 != 7) return 4;

  // Const rvalue ref to literal
  const int&& r5 = 42;
  if (r5 != 42) return 5;

  return 0;
}
