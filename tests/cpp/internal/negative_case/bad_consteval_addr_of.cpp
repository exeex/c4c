// Negative test: taking the address of a consteval function is not allowed.

consteval int add(int a, int b) {
  return a + b;
}

int main() {
  auto p = &add;  // error: cannot take address of consteval function
  return 0;
}
