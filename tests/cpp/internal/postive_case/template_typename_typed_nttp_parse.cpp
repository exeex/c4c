// Parse test: distinguish type template parameters from typed NTTPs whose
// type begins with a dependent typename-qualified-id.

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename T>
struct holder {
    using value_type = T;
};

template <typename T, typename holder<T>::value_type V>
struct typed_value {
    static constexpr holder<T>::value_type value = V;
};

template <typename T, typename enable_if<true, T>::type V>
struct typed_enable_if_value {
    static constexpr T value = V;
};

template <typename T, typename U = typename holder<T>::value_type>
struct type_param_with_default {
    using type = U;
};

int main() {
    type_param_with_default<int>::type x = 1;
    return typed_value<int, 7>::value + typed_enable_if_value<int, 9>::value + x;
}
