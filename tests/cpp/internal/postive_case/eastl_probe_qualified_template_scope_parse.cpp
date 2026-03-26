// Probe: qualified template-id followed by :: in expression context.

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename Ptr>
struct pointer_traits {
    static int to_address(const Ptr&);
    static constexpr bool value = true;
};

template <class Ptr, typename enable_if<pointer_traits<Ptr>::value, int>::type = 0>
int to_address(const Ptr& ptr) {
    return pointer_traits<Ptr>::to_address(ptr);
}

int main() {
    return 0;
}
