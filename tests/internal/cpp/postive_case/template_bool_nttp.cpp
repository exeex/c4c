// Test: in C++ profile, bool/true/false are keywords and bool NTTP works.

template <bool Enabled>
int choose() {
  return Enabled ? 42 : 7;
}

int main() {
  bool yes = true;
  bool no = false;

  if (!yes) return 1;
  if (no) return 2;

  if (choose<true>() != 42) return 3;
  if (choose<false>() != 7) return 4;

  return 0;
}
