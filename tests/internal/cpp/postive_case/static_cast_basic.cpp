// Test: static_cast<T>(expr) support — C++ named cast syntax.
// Verifies that static_cast produces correct type conversions in
// both regular code and consteval contexts.

// Basic integer conversions
int test_int_conversions() {
  long x = 1000000000L;
  int y = static_cast<int>(x);
  if (y != 1000000000) return 1;

  // Narrowing: long → short
  long big = 300;
  short s = static_cast<short>(big);
  if (s != 300) return 2;

  // Widening: char → int
  char c = 65;
  int i = static_cast<int>(c);
  if (i != 65) return 3;

  return 0;
}

// static_cast in expressions
int test_in_expr() {
  int a = 7;
  int b = 2;
  // Integer division, then cast — should still be 3
  long r = static_cast<long>(a / b);
  if (r != 3) return 1;

  return 0;
}

// static_cast in consteval context
consteval int const_narrow(long val) {
  return static_cast<int>(val);
}

consteval long const_widen(int val) {
  return static_cast<long>(val);
}

// static_cast inside template
template <typename T>
T cast_to(long val) {
  return static_cast<T>(val);
}

int main() {
  int r;

  r = test_int_conversions();
  if (r != 0) return r;

  r = test_in_expr();
  if (r != 0) return 10 + r;

  // consteval with static_cast
  int cv1 = const_narrow(42L);
  if (cv1 != 42) return 20;

  long cv2 = const_widen(99);
  if (cv2 != 99) return 21;

  // template + static_cast
  short s = cast_to<short>(500L);
  if (s != 500) return 30;

  int i = cast_to<int>(123456L);
  if (i != 123456) return 31;

  return 0;
}
