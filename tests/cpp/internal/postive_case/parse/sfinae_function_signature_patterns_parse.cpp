// Parse-only regression: free and member function signatures should accept
// SFINAE-shaped return and parameter types through the normal declarator path.
// RUN: %c4cll --parse-only %s

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename T>
struct trait {
    static constexpr bool value = true;
};

template <typename T>
using enable_if_t = typename enable_if<trait<T>::value, int>::type;

template <typename T>
auto free_return(T) -> enable_if_t<T>;

template <typename T>
int free_param(T, enable_if_t<T>* = nullptr);

struct SignatureProbe {
    template <typename T>
    auto member_return(T) -> enable_if_t<T>;

    template <typename T>
    int member_param(T, enable_if_t<T>* = nullptr);
};

template <typename T>
auto SignatureProbe::member_return(T) -> enable_if_t<T> {
    return 0;
}

template <typename T>
int SignatureProbe::member_param(T, enable_if_t<T>*) {
    return 0;
}

int main() {
    SignatureProbe probe;
    return probe.member_return(0) + probe.member_param(0) + free_param(0);
}
