template <typename T, typename U>
consteval int is_same() {
  return __builtin_types_compatible_p(T, U);
}

template <typename T>
constexpr int type_score() {
  if constexpr (is_same<T, int>()) {
    return 42;
  } else {
    return 7;
  }
}

int main() {
  int result = type_score<int>();
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
