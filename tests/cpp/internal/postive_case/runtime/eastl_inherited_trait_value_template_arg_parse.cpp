// Regression: parsing a template base with a dependent trait member expression
// as an NTTP must stop at the outer template close instead of consuming it.

namespace eastl {

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    using type = integral_constant<T, v>;
};

template <typename T>
struct arithmetic {
    static constexpr bool value = true;
};

template <>
struct arithmetic<void> {
    static constexpr bool value = false;
};

template <typename T>
struct is_arithmetic : integral_constant<bool, arithmetic<T>::value> {};

template <typename T, bool = is_arithmetic<T>::value>
struct helper {};

}  // namespace eastl

int main() {
    eastl::helper<int> h;
    (void)h;
    return 0;
}
