// Runtime test: functions defined in anonymous namespaces must be callable
// without qualification from the enclosing scope.
namespace {
  int secret_val = 42;
  int get_secret() { return secret_val; }
  int add(int a, int b) { return a + b; }
}

int main() {
  // Unqualified call to anonymous-namespace function
  if (get_secret() != 42) return 1;
  if (add(3, 7) != 10) return 2;
  // Direct access to anonymous-namespace variable (already works)
  if (secret_val != 42) return 3;
  return 0;
}
