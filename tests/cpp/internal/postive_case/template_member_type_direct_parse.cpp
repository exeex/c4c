// Parse-only regression: direct nested member typedef access on a template
// instantiation should be usable as a type.

template <bool B>
struct bool_constant {
  static constexpr bool value = B;
  using type = bool_constant<B>;
};

int main() {
  bool_constant<true>::type x;
  return x.value ? 0 : 1;
}
