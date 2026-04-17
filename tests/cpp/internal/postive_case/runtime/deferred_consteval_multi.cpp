// Test: multiple consteval calls inside a function that is only instantiated
// by the HIR compile-time reduction pass (truly deferred consteval).
//
// Chain: main → wrapper<int> → compute<int> → {get_size<int>, get_align<int>}
//
// wrapper<int> is a depth-0 template call (collected in Phase 1.6).
// compute<int> is a nested template call (deferred to the HIR pass).
// get_size<int> and get_align<int> are consteval calls inside the deferred function.
//
// Both consteval calls are created as PendingConstevalExpr during deferred
// lowering and evaluated by the compile-time reduction pass.

template <typename T>
consteval int get_size() {
  return sizeof(T);
}

template <typename T>
consteval int get_align() {
  return sizeof(T);  // simplified: align == size for basic types
}

template <typename T>
int compute() {
  return get_size<T>() + get_align<T>();
}

template <typename T>
int wrapper() {
  return compute<T>();
}

int main() {
  int result = wrapper<int>();
  // sizeof(int) + sizeof(int) == 4 + 4 == 8
  if (result == 8) return 0;
  return 1;
}
