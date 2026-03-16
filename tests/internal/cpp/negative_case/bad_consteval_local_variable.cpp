// Negative test: consteval cannot be applied to a local variable declaration.

int main() {
  consteval int x = 42;
  return x;
}
