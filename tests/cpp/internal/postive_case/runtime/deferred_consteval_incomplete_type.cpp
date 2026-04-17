// Test: deferred consteval should revisit sizeof(T) after T becomes complete
// later in the translation unit.
//
// Chain: main -> wrapper<TensorDesc> -> get_size<TensorDesc> (consteval)
//
// The first call site only sees a forward declaration for TensorDesc, but the
// whole translation unit later provides the full layout. The compile-time
// engine should use that late-known layout and reduce the pending consteval.

template <typename T>
consteval int get_size() {
  return sizeof(T);
}

template <typename T>
int wrapper() {
  return get_size<T>();
}

struct TensorDesc;

int main() {
  int result = wrapper<TensorDesc>();
  if (result == 64) return 0;
  return 1;
}

struct TensorDesc {
  int lanes[16];
};
