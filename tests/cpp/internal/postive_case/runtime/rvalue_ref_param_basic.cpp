// Phase 0: Rvalue reference function parameters.
// Functions can accept T&& and modify the referred temporary.

void increment(int&& val) {
  val = val + 1;
}

int consume(int&& val) {
  return val * 2;
}

int main() {
  // Pass literal to rvalue ref param
  int x = 10;
  // Rvalue ref param accepts temporaries — pass expression result
  if (consume(5) != 10) return 1;
  if (consume(x + 1) != 22) return 2;

  return 0;
}
