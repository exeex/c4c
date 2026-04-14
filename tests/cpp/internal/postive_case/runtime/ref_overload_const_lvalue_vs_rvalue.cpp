// Phase 2: Overload resolution for const T& vs T&&.

int g_which = 0;

void f(const int& x) { g_which = 1; }
void f(int&& x) { g_which = 2; }

int main() {
  int a = 42;

  // lvalue arg → f(const int&)
  f(a);
  if (g_which != 1) return 1;

  // rvalue literal → f(int&&) (preferred over const int& for rvalues)
  f(42);
  if (g_which != 2) return 2;

  // rvalue expression → f(int&&)
  f(a + 1);
  if (g_which != 2) return 3;

  return 0;
}
