// Regression: a dependent C-style cast target such as `(T&&)value` must obey
// reference collapsing before downstream overload resolution for the lvalue
// forwarding-reference specialization.

int pick(int&) { return 1; }
int pick(int&&) { return 2; }

template <class T>
int forward_pick(T&& value) {
  return pick((T&&)value);
}

int main() {
  int x = 0;

  if (forward_pick<int&>((int&)x) != 1) return 1;

  return 0;
}
