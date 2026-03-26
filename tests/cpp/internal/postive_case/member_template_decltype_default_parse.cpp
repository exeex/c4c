// Parse-only regression: struct member templates with unnamed type template
// parameters defaulted via decltype(...) should not lose the enclosing class
// body when the decltype contains nested template-ids.

template <typename T>
T declval();

struct true_type {};

struct probe {
  template <typename T, typename = decltype(declval<T&>().~T())>
  static true_type test(int);
};

int main() { return 0; }
