// Parse-only baseline for Step 3 timeout reduction work.
// Clone this probe upward when adding shapes that try to reproduce the
// remaining EASTL memory/tuple parser timeout.
// RUN: %c4cll --parse-only %s

template <bool B, typename T, typename F>
struct conditional {
  using type = T;
};

template <typename T>
struct remove_reference {
  using type = T;
};

template <typename T>
struct add_lvalue_reference {
  using type = T&;
};

template <typename T>
struct is_lvalue_reference {
  static constexpr bool value = false;
};

template <typename T>
struct is_lvalue_reference<T&> {
  static constexpr bool value = true;
};

template <typename A, typename B>
struct pair {
  A first;
  B second;
};

template <unsigned long I, typename Pair>
struct tuple_element;

template <typename A, typename B>
struct tuple_element<0, pair<A, B>> {
  using type = A;
};

template <typename A, typename B>
struct tuple_element<1, pair<A, B>> {
  using type = B;
};

template <unsigned long I, typename Pair>
using tuple_element_t = typename tuple_element<I, Pair>::type;

template <typename T>
struct box {
  using type = T;
};

template <typename T>
using alias_t = typename box<T>::type;

template <typename T>
using const_tuple_element_t = typename conditional<
    is_lvalue_reference<tuple_element_t<0, pair<T, alias_t<T>>>>::value,
    typename add_lvalue_reference<
        const typename remove_reference<
            tuple_element_t<0, pair<T, alias_t<T>>>>::type>::type,
    const tuple_element_t<0, pair<T, alias_t<T>>>>::type;

struct sink {
  const_tuple_element_t<int> left();
  tuple_element_t<1, pair<int, alias_t<int>>> right();
};

int main() {
  return 0;
}
