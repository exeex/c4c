// Runtime regression: a template-derived temporary that inherits operator()
// must lower as construction followed by one member call, not a nested call.

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    using type = integral_constant<T, v>;

    constexpr T operator()() const { return value; }
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T>
struct helper : bool_constant<true> {};

template <typename T>
struct trait : helper<T>::type {};

int main() {
    return trait<int>{}() ? 0 : 1;
}
