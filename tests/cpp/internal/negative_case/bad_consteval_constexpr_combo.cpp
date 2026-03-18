// Negative test: consteval and constexpr cannot both appear on the same function.

consteval constexpr int add(int a, int b) {
  return a + b;
}

int main() {
  return add(1, 2) - 3;
}
