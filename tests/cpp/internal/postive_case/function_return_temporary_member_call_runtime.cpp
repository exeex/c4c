// Regression: function-return temporaries must preserve prvalue receiver state
// so ref-qualified member dispatch picks the `&&` overload.

struct Box {
  int value;

  Box(int&& v) : value(v) {}

  int read() & { return value + 10; }
  int read() && { return value + 20; }
};

Box make_box() { return Box(7); }

int main() {
  Box local(3);

  if (local.read() != 13) return 1;
  if (make_box().read() != 27) return 2;

  return 0;
}
