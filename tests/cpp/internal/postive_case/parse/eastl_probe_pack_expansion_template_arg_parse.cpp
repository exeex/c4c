// Probe: pack expansion nested inside template argument lists.

template <typename... Ts>
struct __and_ {
    static constexpr bool value = true;
};

template <typename T>
struct is_default_constructible {
    static constexpr bool value = true;
};

template <typename T>
struct is_implicitly_default_constructible {
    static constexpr bool value = true;
};

template <typename... Types>
struct tuple_constraints {
    static constexpr bool is_explicitly_default_constructible() {
        return __and_<is_default_constructible<Types>...,
                      __and_<is_implicitly_default_constructible<Types>...>>::value;
    }
};

int main() {
    return 0;
}
