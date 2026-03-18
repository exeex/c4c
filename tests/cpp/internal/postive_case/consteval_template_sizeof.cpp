// Test: consteval template function using sizeof(T) in if constexpr.
// After template-substituted constant evaluation, sizeof(T) should resolve
// to the concrete type size, enabling if constexpr branch elimination.

template <typename T>
consteval int type_size_category() {
  if constexpr (sizeof(T) == 1) {
    return 1;
  } else if constexpr (sizeof(T) == 4) {
    return 4;
  } else if constexpr (sizeof(T) == 8) {
    return 8;
  } else {
    return 0;
  }
}

template <typename T>
consteval T identity(T v) {
  return v;
}

template <typename T>
consteval int scaled(T v) {
  return v * sizeof(T);
}

int main() {
  // sizeof(char) == 1
  int cat_char = type_size_category<char>();
  if (cat_char != 1) return 1;

  // sizeof(int) == 4
  int cat_int = type_size_category<int>();
  if (cat_int != 4) return 2;

  // sizeof(long) == 8
  int cat_long = type_size_category<long>();
  if (cat_long != 8) return 3;

  // consteval template with simple arithmetic
  int id = identity<int>(42);
  if (id != 42) return 4;

  // consteval template using sizeof(T) in arithmetic
  int s = scaled<int>(10);  // 10 * sizeof(int) = 10 * 4 = 40
  if (s != 40) return 5;

  return 0;
}
