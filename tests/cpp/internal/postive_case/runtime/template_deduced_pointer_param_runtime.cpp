// Runtime regression: substituting a deduced template type into a signature
// must preserve outer pointer declarators like T*.

template <typename T>
T load_value(T* ptr) {
  return *ptr;
}

int main() {
  int value = 123;
  return load_value(&value) == 123 ? 0 : 1;
}
