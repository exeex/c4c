// Regression: deferred builtin trait folding should preserve qualified
// user-defined enum types when inherited trait wrappers read the base `::value`.

namespace ns {

enum Color {
  Red,
  Blue,
};

}  // namespace ns

template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
};

template <typename T>
struct is_enum : integral_constant<bool, __is_enum(T)> {};

int main() {
  return is_enum<ns::Color>::value ? 0 : 3;
}
