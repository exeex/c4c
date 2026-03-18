// Test: function templates with non-int integral NTTP parameter types.
// Current coverage is mostly template<int N>; this extends it to unsigned and long.

template <unsigned N>
unsigned add_unsigned(unsigned x) {
  return x + N;
}

template <long N>
long add_long(long x) {
  return x + N;
}

template <unsigned A, long B>
long sum_mixed() {
  return (long)A + B;
}

int main() {
  if (add_unsigned<2U>(40U) != 42U) return 1;
  if (add_unsigned<0U>(42U) != 42U) return 2;

  if (add_long<2>(40L) != 42L) return 3;
  if (add_long<-58>(100L) != 42L) return 4;

  if (sum_mixed<2U, 40>() != 42L) return 5;
  if (sum_mixed<100U, -58>() != 42L) return 6;

  return 0;
}
