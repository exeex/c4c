template <class T>
struct holder {
  using value_type = T;
};

template <class T>
int probe(void* raw, T v) {
  using Alias = typename holder<T>::value_type;
  Alias* p = (typename holder<T>::value_type*)raw;
  Alias w = (Alias)v;
  return p ? w : 0;
}

int main() {
  int x = 5;
  return probe<int>(&x, x) == 5 ? 0 : 1;
}
