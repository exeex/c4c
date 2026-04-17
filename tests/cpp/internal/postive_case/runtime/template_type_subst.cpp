// Test: template type parameter substitution in function signatures and locals.
// Verifies that return types, parameter types, and local variable types are
// correctly substituted for non-int types (long, short, char, etc.).

template <typename T>
T add(T lhs, T rhs) {
  return lhs + rhs;
}

template <typename T>
T identity(T val) {
  return val;
}

// Local variable of type T inside template body.
template <typename T>
T double_val(T x) {
  T result = x + x;
  return result;
}

// T* pointer parameter — must preserve ptr_level.
template <typename T>
T get_first(T* arr) {
  return arr[0];
}

int main() {
  // long: would fail if T is treated as i32 (overflow/truncation)
  long r1 = add<long>(2000000000L, 2000000000L);
  if (r1 != 4000000000L) return 1;

  // short: verify correct narrow type
  short r2 = add<short>(100, 200);
  if (r2 != 300) return 2;

  // char
  char r3 = add<char>(60, 5);
  if (r3 != 65) return 3;

  // identity with long
  long r4 = identity<long>(9876543210L);
  if (r4 != 9876543210L) return 4;

  // local variable of type T
  long r5 = double_val<long>(3000000000L);
  if (r5 != 6000000000L) return 5;

  // T* pointer parameter
  long arr[] = {42L, 99L};
  long r6 = get_first<long>(arr);
  if (r6 != 42L) return 6;

  return 0;
}
