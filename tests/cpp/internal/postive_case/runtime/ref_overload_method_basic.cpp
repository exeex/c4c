// Phase 2: Method overload resolution for T& vs T&&.

struct Obj {
  int which;
  void set(int& x) { which = 1; }
  void set(int&& x) { which = 2; }
};

int main() {
  Obj o;
  o.which = 0;

  int a = 42;

  // lvalue arg → set(int&)
  o.set(a);
  if (o.which != 1) return 1;

  // rvalue literal → set(int&&)
  o.set(42);
  if (o.which != 2) return 2;

  // rvalue expression → set(int&&)
  o.set(a + 1);
  if (o.which != 2) return 3;

  return 0;
}
