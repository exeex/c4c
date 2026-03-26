// Parse-only regression: alias-template NTTP expressions should use the
// canonical template-argument parser rather than local angle-depth scanning.

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

int main() {
  helper<int>::type x;
  return x.value ? 0 : 1;
}
