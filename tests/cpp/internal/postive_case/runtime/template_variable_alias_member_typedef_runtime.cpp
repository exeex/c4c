namespace ns {

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <bool B, typename T, typename F>
struct conditional {
  using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
  using type = F;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

template <typename T, typename U>
struct is_same {
  static constexpr bool value = false;
};

template <typename T>
struct is_same<T, T> {
  static constexpr bool value = true;
};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

template <typename T>
struct add_lvalue_reference {
  using type = T&;
};

template <typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template <typename T>
struct is_reference {
  static constexpr bool value = false;
};

template <typename T>
struct is_reference<T&> {
  static constexpr bool value = true;
};

template <typename T>
constexpr bool is_reference_v = is_reference<T>::value;

}  // namespace ns

int main() {
  if (!ns::is_same_v<ns::enable_if_t<true, short>, short>)
    return 1;
  if (!ns::is_same_v<ns::conditional_t<true, int, long>, int>)
    return 2;
  if (!ns::is_same_v<ns::conditional_t<false, int, long>, long>)
    return 3;
  if (!ns::is_same_v<ns::add_lvalue_reference_t<int>, int&>)
    return 4;
  if (!ns::is_reference_v<ns::add_lvalue_reference_t<int>>)
    return 5;
  return 0;
}
