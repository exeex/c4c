// Test: sizeof and alignof as constant expressions in constexpr/if constexpr.

constexpr int ptr_size = sizeof(void*);
constexpr int int_size = sizeof(int);
constexpr int long_size = sizeof(long);
constexpr int int_align = alignof(int);
constexpr int long_align = alignof(long);

int check_sizeof() {
  if constexpr (sizeof(int) == 4) {
    return 1;  // expected on LP64
  } else {
    return 0;
  }
}

int check_alignof() {
  if constexpr (alignof(long) == 8) {
    return 1;  // expected on LP64
  } else {
    return 0;
  }
}

int check_sizeof_expr() {
  int x;
  if constexpr (sizeof(x) == 4) {
    return 1;  // sizeof(int) == 4
  } else {
    return 0;
  }
}

int main() {
  // Verify constexpr globals folded correctly (LP64 assumptions).
  if (ptr_size != 8) return 1;
  if (int_size != 4) return 2;
  if (long_size != 8) return 3;
  if (int_align != 4) return 4;
  if (long_align != 8) return 5;

  // Verify if constexpr with sizeof/alignof conditions.
  if (check_sizeof() != 1) return 6;
  if (check_alignof() != 1) return 7;
  if (check_sizeof_expr() != 1) return 8;

  return 0;
}
