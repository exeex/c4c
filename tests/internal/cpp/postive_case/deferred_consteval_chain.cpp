// Test: consteval call inside a function that is only instantiated by the
// HIR compile-time reduction pass (deferred template instantiation).
//
// Chain: main → outer<int> → middle<int> → get_size<int> (consteval)
//
// outer<int> is a depth-0 template call (collected in Phase 1.6).
// middle<int> is a nested template call (deferred to the HIR pass).
// get_size<int> is a consteval call inside the deferred function.
//
// The consteval reduction of get_size<int> can only happen after the HIR pass
// instantiates middle<int>.  This proves the HIR pass can unlock consteval
// reductions through deferred template instantiation.

template <typename T>
consteval int get_size() {
  return sizeof(T);
}

template <typename T>
int middle() {
  return get_size<T>();
}

template <typename T>
int outer() {
  return middle<T>();
}

int main() {
  int result = outer<int>();
  if (result == 4) return 0;
  return 1;
}
