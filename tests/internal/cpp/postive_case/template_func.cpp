template <typename T>
T add(T lhs, T rhs) {
  return lhs + rhs;
}

int main() {
  int result = add<int>(20, 22);
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
