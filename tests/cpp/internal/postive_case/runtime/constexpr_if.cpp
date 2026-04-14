template <typename T>
constexpr int type_tag(T value) {
  if constexpr (sizeof(T) == sizeof(char)) {
    return value;
  } else {
    return value * 2;
  }
}

int main() {
  int result = type_tag(21);
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
