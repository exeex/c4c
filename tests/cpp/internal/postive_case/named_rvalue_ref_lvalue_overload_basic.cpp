// Regression: a named `T&&` variable is still an lvalue expression, so plain
// use should pick lvalue overloads while explicit cast/forward restores rvalue
// behavior.

template <class T>
T&& forward_value(T& value) {
  return static_cast<T&&>(value);
}

int pick(int& value) { return value + 1; }
int pick(int&& value) { return value + 2; }

int main() {
  int&& ref = 40 + 1;

  if (pick(ref) != 42) return 1;
  if (pick(static_cast<int&&>(ref)) != 43) return 2;
  if (pick(forward_value<int>(ref)) != 43) return 3;

  return 0;
}
