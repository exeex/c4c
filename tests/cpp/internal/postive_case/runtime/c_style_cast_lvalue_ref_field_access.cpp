// Regression: field access through a C-style cast to lvalue reference must
// preserve the original object identity for both reads and writes.

struct Box {
  int value;
};

int main() {
  Box b{1};

  if (((Box&)b).value != 1) return 1;

  ((Box&)b).value = 7;

  return b.value == 7 ? 0 : 2;
}
