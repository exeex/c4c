// Regression: local `auto&&` initialized from an lvalue should deduce to an
// lvalue reference, preserve aliasing, and stay an lvalue in overload
// resolution.

int pick(int& value) { return value + 1; }
int pick(int&& value) { return value + 2; }

int main() {
  int x = 40;
  auto&& ref = x;

  ref = 41;
  if (x != 41) return 1;

  if (pick(ref) != 42) return 2;

  return 0;
}
