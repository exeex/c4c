// Phase 0: Basic rvalue reference declaration and usage.
// T&& can bind to rvalues (literals, temporaries).

int main() {
  // Rvalue ref binding to a literal
  int&& r1 = 42;
  if (r1 != 42) return 1;

  // Modify through rvalue ref
  r1 = 100;
  if (r1 != 100) return 2;

  // Rvalue ref to a different type
  long&& r2 = 999L;
  if (r2 != 999) return 3;

  // Const rvalue ref
  const int&& r3 = 7;
  if (r3 != 7) return 4;

  return 0;
}
