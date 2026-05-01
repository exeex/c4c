template <typename T>
struct Outer {
  using Alias = T;
};

template <typename T>
consteval int alias_width() {
  return sizeof(typename Outer<T>::Alias);
}

int main() {
  if (alias_width<char>() != 1) return 1;
  if (alias_width<long>() != 8) return 2;
  return 0;
}
