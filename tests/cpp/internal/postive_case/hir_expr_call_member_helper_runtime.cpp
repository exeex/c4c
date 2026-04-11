// HIR regression: constructor, member-call, operator-call, and ref-qualified
// dispatch paths should keep working while call/member lowering is factored
// into helpers.

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
