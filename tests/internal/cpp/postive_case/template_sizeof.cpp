// Test: sizeof(T) in template bodies must use the substituted type.
// Verifies that sizeof resolves correctly for char (1), short (2), int (4),
// long (8), and pointer types in template contexts.

template<typename T>
int get_size() {
  return sizeof(T);
}

template<typename T>
int get_align() {
  return __alignof__(T);
}

// sizeof applied to a template-typed local variable
template<typename T>
int local_sizeof() {
  T val;
  return sizeof(val);
}

int main() {
  // sizeof(T) tests
  if (get_size<char>() != 1) return 1;
  if (get_size<short>() != 2) return 2;
  if (get_size<int>() != 4) return 3;
  if (get_size<long>() != 8) return 4;
  if (get_size<long long>() != 8) return 5;

  // pointer sizes (all 8 on 64-bit)
  if (get_size<int*>() != 8) return 6;
  if (get_size<char*>() != 8) return 7;

  // alignof(T) tests
  if (get_align<char>() != 1) return 10;
  if (get_align<int>() != 4) return 11;
  if (get_align<long>() != 8) return 12;

  // sizeof(local_var) where var has type T
  if (local_sizeof<char>() != 1) return 20;
  if (local_sizeof<long>() != 8) return 21;

  return 0;
}
