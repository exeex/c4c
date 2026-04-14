// Regression: variable-template initializers backed by inherited dependent
// trait values must resolve through the instantiated helper/base chain.

template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
  using type = integral_constant<T, v>;
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T>
struct arithmetic {
  static constexpr bool value = true;
};

template <>
struct arithmetic<void> {
  static constexpr bool value = false;
};

template <typename T, bool = arithmetic<T>::value>
struct is_signed_helper : bool_constant<(T(-1) < T(0))> {};

template <typename T>
struct is_signed_helper<T, false> : bool_constant<false> {};

template <typename T>
struct is_signed : is_signed_helper<T>::type {};

template <typename T>
constexpr bool is_signed_v = is_signed<T>::value;

template <typename T>
int check_signed() {
  return is_signed_v<T> ? 0 : 1;
}

int main() {
  if (check_signed<int>() != 0)
    return 1;
  if (check_signed<unsigned>() != 1)
    return 2;
  return 0;
}
