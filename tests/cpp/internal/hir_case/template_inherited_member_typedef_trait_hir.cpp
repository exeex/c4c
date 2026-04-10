// HIR regression: inherited trait-style member typedef lookup should stay
// concrete when the owner resolves through a realized base struct.

namespace ns {

template <typename T>
struct type_identity {
  using type = T;
};

namespace internal {

template <typename T>
auto try_add_lvalue_reference(int) -> type_identity<T&>;

template <typename T>
auto try_add_lvalue_reference(...) -> type_identity<T>;

}  // namespace internal

template <typename T>
struct add_lvalue_reference : decltype(internal::try_add_lvalue_reference<T>(0)) {
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
  return ns::is_reference_v<ns::add_lvalue_reference_t<int>> ? 0 : 9;
}
