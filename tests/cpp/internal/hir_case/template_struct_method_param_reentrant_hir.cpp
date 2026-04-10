// HIR regression: re-entrant template struct method lowering must reserve
// callable slots even when an outer method's parameter type realization lowers
// another instantiated template method first.

template<typename T>
struct Inner {
  T value;

  T get() const {
    return value;
  }
};

template<typename T>
struct Outer {
  void absorb(Inner<T> inner) {
    (void)inner.get();
  }
};

int main() {
  Outer<int> outer;
  (void)outer;
  return 0;
}
