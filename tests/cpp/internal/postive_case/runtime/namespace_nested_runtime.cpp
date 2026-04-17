// Runtime test: nested namespace functions should have correct LLVM IR names.
namespace outer {
namespace inner {
int mul(int a, int b) { return a * b; }
}
}

int main() {
  if (outer::inner::mul(5, 6) != 30) return 1;
  return 0;
}
