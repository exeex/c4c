// HIR regression: deferred template member typedef resolution should keep a
// nested owner chain concrete once the owner struct work is realized.

template <typename T>
struct box {
  using value_type = T;
};

template <typename T>
struct holder {
  using inner = typename box<T>::value_type;
};

template <typename T>
struct outer {
  typename holder<T>::inner value;
};

int main() {
  outer<int> x;
  x.value = 42;
  return x.value - 42;
}
