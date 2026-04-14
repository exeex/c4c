// Runtime coverage for NTTP defaults selecting the correct specialization.

struct true_type { static const bool value = 1; };
struct false_type { static const bool value = 0; };

template <typename T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};
template <> struct is_void<const void> : true_type {};
template <> struct is_void<volatile void> : true_type {};
template <> struct is_void<const volatile void> : true_type {};

template <typename T, bool = is_void<T>::value || is_void<T>::value>
struct complex_default {
    static const int val = 0;
};

template <typename T>
struct complex_default<T, true> {
    static const int val = 1;
};

template <typename T>
int check_default_value() {
    return complex_default<T>::val;
}

template <typename T>
int check_explicit_true_value() {
    return complex_default<T, true>::val;
}

template <typename T>
int check_explicit_false_value() {
    return complex_default<T, false>::val;
}

int main() {
    if (check_default_value<int>() != 0)
        return 1;
    if (check_default_value<void>() != 1)
        return 2;
    if (check_default_value<const void>() != 1)
        return 3;
    if (check_explicit_true_value<int>() != 1)
        return 4;
    if (check_explicit_false_value<void>() != 0)
        return 5;
    return 0;
}
