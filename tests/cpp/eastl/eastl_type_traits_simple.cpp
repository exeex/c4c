#include <EASTL/type_traits.h>

enum Color {
    Red,
    Blue
};

template <typename T>
int check_signed_traits() {
    if (!eastl::is_signed_v<T>)
        return 10;
    if (eastl::is_unsigned_v<T>)
        return 11;
    return 0;
}

template <typename T>
int check_reference_transforms() {
    if (!eastl::is_same_v<eastl::remove_reference_t<T&>, T>)
        return 20;
    if (!eastl::is_same_v<eastl::remove_cv_t<const volatile T>, T>)
        return 21;
    if (!eastl::is_reference_v<eastl::add_lvalue_reference_t<T>>)
        return 22;
    return 0;
}

int main() {
    int rc = check_signed_traits<int>();
    if (rc != 0)
        return rc;

    rc = check_reference_transforms<int>();
    if (rc != 0)
        return rc;

    if (!eastl::is_integral<char>::value)
        return 1;
    if (!eastl::is_floating_point<double>::value)
        return 2;
    if (!eastl::is_enum<Color>::value)
        return 3;
    if (!eastl::is_const_v<const int>)
        return 4;
    if (!eastl::is_same_v<eastl::conditional_t<true, int, long>, int>)
        return 5;
    if (!eastl::is_same_v<eastl::enable_if_t<true, short>, short>)
        return 6;
    if (!eastl::is_same_v<eastl::remove_cv_t<const volatile unsigned int>, unsigned int>)
        return 7;
    if (!eastl::is_same_v<eastl::remove_reference_t<int&&>, int>)
        return 8;

    return 0;
}
