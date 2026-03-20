// Compare-mode smoke test: scalar arithmetic, globals, control flow.
int g = 42;

int add(int a, int b) { return a + b; }

int factorial(int n) {
  int r = 1;
  for (int i = 2; i <= n; i++)
    r *= i;
  return r;
}

int main() {
  int x = add(g, 8);
  if (x > 50)
    return 0;
  return factorial(5) == 120 ? 0 : 1;
}
