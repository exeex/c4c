// Test: non-type template parameters (NTTP).
// template<int N> where N is an integer constant.

template <int N>
int get_value() {
  return N;
}

template <int N>
int add_n(int x) {
  return x + N;
}

template <int A, int B>
int sum_params() {
  return A + B;
}

int main() {
  // Basic NTTP usage.
  if (get_value<42>() != 42) return 1;
  if (get_value<0>() != 0) return 2;
  if (get_value<-1>() != -1) return 3;

  // NTTP in expression.
  if (add_n<10>(5) != 15) return 4;
  if (add_n<0>(7) != 7) return 5;

  // Multiple NTTP.
  if (sum_params<3, 4>() != 7) return 6;
  if (sum_params<100, 200>() != 300) return 7;

  return 0;
}
