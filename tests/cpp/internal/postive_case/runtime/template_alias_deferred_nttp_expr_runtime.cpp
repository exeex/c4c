// Minimal runtime probe for the lazy template-struct path:
// alias template + dependent NTTP expression in a base class.

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;

    constexpr T operator()() const { return value; }
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T>
struct signed_probe : bool_constant<(T(-1) < T(0))> {};

int main() {
    if (!signed_probe<int>::value)
        return 1;
    if (signed_probe<unsigned int>::value)
        return 2;

    // operator() dispatch through inherited base requires more
    // infrastructure (inherited method lookup); tested separately.

    return 0;
}
