// Regression: method ref-overload calls must pass the address of the bound object,
// not the integer value coerced to a pointer.

struct Obj {
  int which;
  void set(int& x) { which = x; }
  void set(int&& x) { which = x + 100; }
};

int main() {
  Obj o;
  int a = 42;

  o.set(a);
  if (o.which != 42) return 1;

  o.set(5);
  if (o.which != 105) return 2;

  return 0;
}
