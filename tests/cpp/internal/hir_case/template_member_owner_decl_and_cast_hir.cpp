// HIR regression: a member-typedef chain routed through a template owner
// should stay concrete when reused in both a local declaration and a cast
// target inside an instantiated function body.

template <typename T>
struct box {
  using value_type = T;
};

template <typename T>
struct holder {
  using alias = typename box<T>::value_type;
};

template <typename T>
int probe(typename holder<T>::alias input) {
  using Alias = typename holder<T>::alias;
  Alias local = input;
  return (Alias)local;
}

int main() {
  return probe<int>(42) - 42;
}
