// HIR regression: member typedef lookup through a template owner should lower
// the instantiated field type concretely.

template <typename T>
struct box {
  using value_type = T;
};

template <typename T>
struct wrap {
  using alias = typename box<T>::value_type;
  typename box<T>::value_type value;
  alias mirror;
};

int main() {
  wrap<int> w;
  w.value = 42;
  w.mirror = w.value;
  return w.mirror - 42;
}
