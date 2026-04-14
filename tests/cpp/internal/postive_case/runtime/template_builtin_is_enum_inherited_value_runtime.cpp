// Regression: trait wrappers that inherit `integral_constant<bool, __is_enum(T)>`
// should preserve the enum classification when callers read the inherited
// `::value` member.

template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
};

enum Color {
  Red,
  Blue,
};

template <typename T>
struct is_enum : integral_constant<bool, __is_enum(T)> {};

int main() {
  return is_enum<Color>::value ? 0 : 3;
}
