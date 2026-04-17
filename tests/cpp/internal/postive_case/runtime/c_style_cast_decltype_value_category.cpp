// Regression: decltype should preserve the reference category of
// reference-qualified C-style casts when the result is consumed as a type.

int main() {
  int x = 0;

  decltype((int&)x) lref = (int&)x;
  lref = 7;
  if (x != 7) return 1;

  decltype((int&&)x) rref = (int&&)x;
  rref = 9;
  if (x != 9) return 2;

  return 0;
}
