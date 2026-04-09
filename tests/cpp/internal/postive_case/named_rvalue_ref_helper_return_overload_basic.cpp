// Regression: helper-return paths must preserve the value category implied by
// the helper return type when they forward a named `T&&` parameter into a
// downstream ref-overload set.

int pick(int& value) { return value + 1; }
int pick(int&& value) { return value + 2; }

int& as_lvalue(int&& value) { return value; }
int&& as_rvalue(int&& value) { return static_cast<int&&>(value); }

int main() {
  int&& ref = 40 + 1;

  if (pick(as_lvalue(static_cast<int&&>(ref))) != 42) return 1;
  if (pick(as_rvalue(static_cast<int&&>(ref))) != 43) return 2;

  return 0;
}
