// Regression probe: inherited static member lookup should work through a
// derived trait type, not only through object-call fallbacks.

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    using type = integral_constant<T, v>;

    constexpr operator T() const { return value; }
    constexpr T operator()() const { return value; }
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

template <typename T, bool = arithmetic<T>::value>
struct is_signed_helper : bool_constant<(T(-1) < T(0))> {};

template <typename T>
struct is_signed_helper<T, false> : bool_constant<false> {};

template <typename T>
struct is_signed : is_signed_helper<T>::type {};

int main() {
    if (!is_signed<int>::value)
        return 1;
    if (is_signed<unsigned int>::value)
        return 2;
    if (is_signed<void>::value)
        return 3;
    return 0;
}
