template <typename T>
T add(T lhs, T rhs) {
  return lhs + rhs;
}

template <typename T>
T twice(T value) {
  return add<T>(value, value);
}

int main() {
  int result = twice<int>(21);
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
