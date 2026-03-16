// Negative test: calling a consteval function with a non-constant argument
// must be a compile error.

consteval int square(int x) {
  return x * x;
}

int main() {
  int n = 5;
  int r = square(n);  // error: n is not a constant expression
  return r;
}
