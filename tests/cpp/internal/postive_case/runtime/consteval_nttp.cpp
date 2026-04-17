// Test: consteval with non-type template parameters.
// Combines consteval evaluation with NTTP and mixed type+NTTP params.

template <int N>
consteval int square_n() {
  return N * N;
}

template <typename T, int N>
consteval T add_const(T x) {
  return x + N;
}

template <int A, int B>
consteval int sum_const() {
  return A + B;
}

int main() {
  // consteval with pure NTTP.
  int a = square_n<5>();
  if (a != 25) return 1;

  int b = square_n<-3>();
  if (b != 9) return 2;

  // consteval with mixed type + NTTP.
  int c = add_const<int, 10>(5);
  if (c != 15) return 3;

  long d = add_const<long, 100>(200L);
  if (d != 300) return 4;

  // consteval with multiple NTTP.
  int e = sum_const<7, 8>();
  if (e != 15) return 5;

  return 0;
}
