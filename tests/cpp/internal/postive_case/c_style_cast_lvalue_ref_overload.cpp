// Regression: C-style casts to lvalue reference must preserve lvalue category
// for overload resolution.

int g_which = 0;

void pick(int& x) { g_which = 1; }
void pick(int&& x) { g_which = 2; }

int main() {
  int a = 5;

  pick((int&)a);
  if (g_which != 1) return 1;

  pick((int&&)a);
  if (g_which != 2) return 2;

  return 0;
}
