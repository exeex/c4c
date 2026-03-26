// Parse-only regression: a member template with decltype default arguments
// should leave the parser positioned correctly for following overloads.

template <typename T>
T declval();

struct true_type {};
struct false_type {};

struct probe {
  template <typename T, typename = decltype(declval<T&>().~T())>
  static true_type test(int);

  template <typename>
  static false_type test(...);
};

int main() { return 0; }
