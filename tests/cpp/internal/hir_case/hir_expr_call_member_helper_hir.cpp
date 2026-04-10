// HIR regression: constructor calls, member calls, and ref-qualified
// operator-call lowering should keep the same helper-driven call shape after
// the call-lowering extraction from hir_expr.cpp.

struct Box {
  int value;

  Box(int&& v) : value(v) {}

  int read() & { return value; }
  int operator()(int&& delta) && { return value + delta; }
};

int main() {
  Box local(3);

  if (local.read() != 3) return 1;
  if (Box(4)(6) != 10) return 2;

  return 0;
}
