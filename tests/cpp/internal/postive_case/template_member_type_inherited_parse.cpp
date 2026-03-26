// Parse-only regression: nested member typedef lookup should continue through
// base classes of a template specialization.

template <bool B>
struct bool_constant {
  static constexpr bool value = B;
  using type = bool_constant<B>;
};

template <typename T>
struct helper : bool_constant<true> {};

template <typename T>
struct is_signed : helper<T>::type {};

int main() {
  is_signed<int> x;
  return x.value ? 0 : 1;
}
