// Phase 1: Rvalue reference parameters accept rvalue arguments.

int consume(int&& val) {
  return val + 1;
}

void modify(int&& val) {
  val = val * 2;
}

int forward_add(int&& a, int&& b) {
  return a + b;
}

int main() {
  // Literal args
  if (consume(10) != 11) return 1;
  if (consume(0) != 1) return 2;

  // Expression args (rvalue)
  int x = 5;
  if (consume(x + 1) != 7) return 3;
  if (consume(x * 2) != 11) return 4;

  // Multiple rvalue ref params
  if (forward_add(3, 4) != 7) return 5;

  return 0;
}
