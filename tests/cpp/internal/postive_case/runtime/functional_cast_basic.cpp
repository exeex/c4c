// Test: C++ functional cast syntax T(expr).
int main() {
  int one = bool(3);
  int zero = bool(0);

  if (one != 1) return 1;
  if (zero != 0) return 2;

  long widened = long(7);
  if (widened != 7) return 3;

  return 0;
}
