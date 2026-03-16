// Negative test: consteval cannot be applied to a variable declaration.

consteval int x = 5;

int main() {
  return x;
}
