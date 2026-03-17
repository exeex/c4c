// Test: default template arguments and empty angle brackets <>.
// Verifies that template type and NTTP parameters with defaults work correctly
// when partially or fully omitted at call sites.

// Single type parameter with default.
template <typename T = int>
T zero() {
  return (T)0;
}

// Mixed type + NTTP with defaults.
template <typename T = long, int N = 42>
T get_value() {
  return (T)N;
}

// First param explicit, second defaults.
template <typename T, typename U = int>
T convert(U val) {
  return static_cast<T>(val);
}

// NTTP-only with default.
template <int N = 10>
int scale(int x) {
  return x * N;
}

int main() {
  // Explicit type arg — no default used.
  int r1 = zero<int>();
  if (r1 != 0) return 1;

  long r2 = zero<long>();
  if (r2 != 0) return 2;

  // Empty <> — default type (int) used.
  int r3 = zero<>();
  if (r3 != 0) return 3;

  // All defaults: T=long, N=42.
  long r4 = get_value<>();
  if (r4 != 42L) return 4;

  // Override T only, N defaults to 42.
  int r5 = get_value<int>();
  if (r5 != 42) return 5;

  // Override both.
  short r6 = get_value<short, 10>();
  if (r6 != 10) return 6;

  // First explicit, second defaults to int.
  long r7 = convert<long>(100);
  if (r7 != 100L) return 7;

  // Both explicit.
  long r8 = convert<long, short>(200);
  if (r8 != 200L) return 8;

  // NTTP default.
  int r9 = scale<>(5);
  if (r9 != 50) return 9;

  // NTTP explicit.
  int r10 = scale<3>(5);
  if (r10 != 15) return 10;

  return 0;
}
