// Regression: explicit-template helper calls returning `T&` or `T&&` must keep
// their instantiated reference category through downstream ref-overload
// selection.

template <class T>
T& as_lvalue(T&& value) {
  return value;
}

template <class T>
T&& as_rvalue(T& value) {
  return static_cast<T&&>(value);
}

int pick(int& value) { return value + 1; }
int pick(int&& value) { return value + 2; }

int main() {
  int&& ref = 40 + 1;

  if (pick(as_lvalue<int>(static_cast<int&&>(ref))) != 42) return 1;
  if (pick(as_rvalue<int>(ref)) != 43) return 2;

  return 0;
}
