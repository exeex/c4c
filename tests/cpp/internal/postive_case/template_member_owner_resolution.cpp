// HIR regression: member typedef lookup through a template owner should lower
// the instantiated field type concretely.

template <typename T>
struct box {
  using value_type = T;
};

template <typename T>
struct wrap {
  typename box<T>::value_type value;
};

int main() {
  wrap<int> w;
  w.value = 42;
  return w.value - 42;
}
