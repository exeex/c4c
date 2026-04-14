// Regression: forwarding wrappers must keep distinct `T=int&` and `T=int`
// specializations materialized with distinct emitted identities.

int pick(int& value) { return value + 1; }
int pick(int&& value) { return value + 2; }

template <class T>
int forward_pick(T&& value) {
  return pick((T&&)value);
}

int main() {
  int x = 0;

  if (forward_pick<int&>((int&)x) != 1) return 1;
  if (forward_pick<int>((int&&)x) != 2) return 2;

  return 0;
}
