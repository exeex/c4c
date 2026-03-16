// Test: local function-pointer declarations should be canonicalized
// in ResolvedTypeTable (Phase B).

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }

int apply(int (*op)(int, int), int x, int y) {
  return op(x, y);
}

int main(void) {
  // Local fn-ptr declarations — these should get canonical types.
  int (*fp)(int, int) = add;
  int (*fp2)(int, int);
  fp2 = sub;

  if (fp(3, 4) != 7) return 1;
  if (fp2(10, 3) != 7) return 2;
  if (apply(mul, 6, 7) != 42) return 3;

  // Reassign local fn-ptr
  fp = mul;
  if (fp(5, 8) != 40) return 4;

  return 0;
}
