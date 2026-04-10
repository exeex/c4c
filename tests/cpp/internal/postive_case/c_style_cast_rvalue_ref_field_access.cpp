// Regression: field access through a C-style cast to rvalue reference must
// preserve xvalue category and original object identity.

struct Box {
  int value;
};

int pick(int&) { return 1; }
int pick(int&&) { return 2; }

int main() {
  Box b{1};

  if (((Box&&)b).value != 1) return 1;
  if (pick(((Box&&)b).value) != 2) return 2;

  int&& ref = ((Box&&)b).value;
  ref = 7;

  return b.value == 7 ? 0 : 3;
}
