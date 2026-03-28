template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

template <typename T>
struct signed_probe : bool_constant<(T(-1) < T(0))> {};

int main() {
    if (!signed_probe<int>::value)
        return 1;
    return 0;
}
