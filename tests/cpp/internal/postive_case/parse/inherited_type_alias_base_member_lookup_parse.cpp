// Parse-only regression: inherited `::type` lookup must recurse through base
// aliases such as `false_type`, not only direct struct tags.

namespace eastl {
template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
  using type = integral_constant<T, v>;
};

using false_type = integral_constant<bool, false>;

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
struct is_signed_helper<T, false> : false_type {};

template <typename T>
struct is_signed : is_signed_helper<T>::type {};
}

int main() {
  return eastl::is_signed<void>::value ? 1 : 0;
}
