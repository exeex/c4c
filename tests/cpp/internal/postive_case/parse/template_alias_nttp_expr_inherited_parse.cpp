// Parse-only regression: alias-template NTTP expressions should continue to
// work when the resulting nested type is used in a base-clause.

template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
  using type = integral_constant<T, v>;
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T, bool = true>
struct helper {
  using type = bool_constant<false>;
};

template <typename T>
struct helper<T, true> {
  using type = bool_constant<T(-1) < T(0)>;
};

template <typename T>
struct is_signed : helper<T>::type {};

int main() {
  is_signed<int> x;
  return x.value ? 0 : 1;
}
