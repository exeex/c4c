// HIR regression: a nested template-owner member typedef chain should stay
// concrete when reused for both struct fields and instantiated local vars.

template <typename T>
struct leaf {
  using type = T;
};

template <typename T>
struct middle {
  using alias = typename leaf<T>::type;
};

template <typename T>
struct wrapper {
  using alias = typename middle<T>::alias;
  alias value;
};

template <typename T>
int probe(typename wrapper<T>::alias input) {
  typename wrapper<T>::alias local = input;
  return local;
}

int main() {
  wrapper<int> item;
  item.value = probe<int>(42);
  return item.value - 42;
}
