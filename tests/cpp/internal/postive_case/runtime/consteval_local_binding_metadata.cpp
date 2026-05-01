// Regression coverage for consteval function-body local binding producers.
// Parameters, nested locals, and loop locals must be readable through the
// structured/TextId interpreter binding maps.

consteval int nested_local_shadow(int x) {
  {
    int x = 4;
    if (x != 4) return -1;
  }
  return x;
}

consteval int loop_local_sum(int n) {
  int total = 0;
  for (int i = 0; i < n; i = i + 1) {
    total = total + i;
  }
  return total;
}

int main() {
  if (nested_local_shadow(9) != 9) return 1;
  if (loop_local_sum(5) != 10) return 2;
  return 0;
}
