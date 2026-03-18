// Test: explicit template specialization.
// template<> T foo<T>(...) overrides the generic template for specific types.

template <typename T>
T add(T a, T b) {
  return a + b;
}

// Explicit specialization for int — adds 100.
template <>
int add<int>(int a, int b) {
  return a + b + 100;
}

template <typename T>
T identity(T val) {
  return val;
}

// Explicit specialization for char — returns 'X' always.
template <>
char identity<char>(char val) {
  return 'X';
}

int main() {
  // add<int> should use specialization: 1 + 2 + 100 = 103
  int r1 = add<int>(1, 2);
  if (r1 != 103) return 1;

  // add<long> should use generic: 10 + 20 = 30
  long r2 = add<long>(10L, 20L);
  if (r2 != 30L) return 2;

  // identity<char> should use specialization: always 'X'
  char r3 = identity<char>('A');
  if (r3 != 'X') return 3;

  // identity<int> should use generic: returns input
  int r4 = identity<int>(42);
  if (r4 != 42) return 4;

  return 0;
}
