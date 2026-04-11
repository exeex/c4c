// HIR regression: deferred template member typedef resolution should keep the
// instantiated owner concrete across a function signature and local variable.

template <typename T>
struct leaf {
  using type = T;
};

template <typename T>
struct wrapper {
  using alias = typename leaf<T>::type;
};

template <typename T>
typename wrapper<T>::alias bounce(typename wrapper<T>::alias input) {
  typename wrapper<T>::alias local = input;
  return local;
}

int main() {
  return bounce<int>(42) - 42;
}
