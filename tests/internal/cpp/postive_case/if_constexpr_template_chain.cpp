template <typename T>
consteval int is_char_sized() {
  return __builtin_types_compatible_p(T, char);
}

template <typename T>
constexpr int classify(T value) {
  if constexpr (is_char_sized<T>()) {
    return value;
  } else if constexpr (sizeof(T) == sizeof(int)) {
    return value * 2;
  } else {
    return 0;
  }
}

int main() {
  int result = classify<int>(21);
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
