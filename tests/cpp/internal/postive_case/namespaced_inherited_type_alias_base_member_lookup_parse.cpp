// Parse-only regression: `typename ns::Template<T>::type` must keep the
// templated owner intact so later `::type` lookup can resolve through an
// inherited alias-base path.

namespace eastl {
template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
  typedef integral_constant<T, v> type;
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

typedef integral_constant<bool, false> false_type;

template <typename T, bool = arithmetic<T>::value>
struct is_signed_helper : bool_constant<T(-1) < T(0)> {};

template <typename T>
struct is_signed_helper<T, false> : false_type {};

template <typename T>
int use_signed_type() {
  typename eastl::is_signed_helper<T>::type value{};
  return value.value ? 1 : 0;
}
}  // namespace eastl

int main() {
  return eastl::use_signed_type<int>();
}
