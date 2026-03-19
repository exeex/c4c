// Regression: static_cast<T&&>(lvalue) should produce an rvalue/xvalue-like
// expression for overload selection and rvalue-reference binding.

int g_which = 0;

void pick(int& x) { g_which = 1; }
void pick(int&& x) { g_which = 2; }

int consume(int&& x) { return x + 1; }

int main() {
  int a = 41;

  pick(static_cast<int&&>(a));
  if (g_which != 2) return 1;

  if (consume(static_cast<int&&>(a)) != 42) return 2;

  return 0;
}
