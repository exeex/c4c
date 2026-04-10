// HIR regression: lowering `function().method()` must preserve the
// temporary-producing base call so ref-qualified member dispatch keeps the
// prvalue receiver path.

struct Box {
  int value;

  Box(int&& v) : value(v) {}

  int read() & { return value + 10; }
  int read() && { return value + 20; }
};

Box make_box() { return Box(7); }

int main() {
  Box local(3);
  return local.read() + make_box().read();
}
