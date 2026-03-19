// Runtime test: namespace-qualified function calls should work correctly.
namespace math {
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
}

int main() {
  if (math::add(3, 4) != 7) return 1;
  if (math::sub(10, 3) != 7) return 2;
  return 0;
}
