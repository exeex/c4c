// Test: if constexpr with compile-time-known conditions folds correctly.
// The condition is evaluable during HIR lowering (no template dependency).

constexpr int mode = 2;

int compute(int x) {
  if constexpr (mode == 1) {
    return x + 10;
  } else if constexpr (mode == 2) {
    return x * 2;
  } else {
    return 0;
  }
}

int main() {
  int result = compute(21);
  if (result == 42) return 0;
  return 1;
}
