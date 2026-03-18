// Test: local const/constexpr values usable in switch case labels
int main() {
  const int A = 1;
  const int B = A + 1;
  constexpr int C = B + 1;

  int x = 2;
  int result = -1;
  switch (x) {
    case A: result = 10; break;
    case B: result = 20; break;
    case C: result = 30; break;
    default: result = 99; break;
  }
  // x == 2 == B, so result should be 20
  if (result == 20) return 0;
  return 1;
}
