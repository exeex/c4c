// Template argument deduction: deduce type parameters from call arguments.

template <typename T>
T add(T lhs, T rhs) {
  return lhs + rhs;
}

template <typename T>
T identity(T x) {
  return x;
}

// Deduction with pointer parameter
template <typename T>
T deref(T* ptr) {
  return *ptr;
}

int main() {
  // Deduce T = int from integer literal arguments
  int r1 = add(20, 22);
  if (r1 != 42) return 1;

  // Deduce T = long from variable arguments
  long a = 100L;
  long b = 200L;
  long r2 = add(a, b);
  if (r2 != 300L) return 2;

  // Deduce T = int from single arg
  int r3 = identity(7);
  if (r3 != 7) return 3;

  // Mix deduced and explicit: explicit still works
  int r4 = add<int>(1, 2);
  if (r4 != 3) return 4;

  // Deduce T = int from pointer parameter
  int val = 99;
  int r5 = deref(&val);
  if (r5 != 99) return 5;

  return 0;
}
