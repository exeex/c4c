// Parse regression: cover the first-wave SFINAE template-parameter families in
// both top-level and record-member template preludes.

namespace std {
template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;
}  // namespace std

template <typename T>
struct trait {
    static constexpr bool value = true;
};

template <typename T, std::enable_if_t<trait<T>::value, int> = 0>
struct unnamed_typed_nttp {
    static constexpr int value = 1;
};

template <typename T, std::enable_if_t<trait<T>::value, int> Dummy = 0>
struct named_typed_nttp {
    static constexpr int value = Dummy + 2;
};

template <typename T, typename = std::enable_if_t<trait<T>::value>>
struct unnamed_type_default {
    using type = T;
};

struct member_scope_probe {
    template <typename T, std::enable_if_t<trait<T>::value, int> = 0>
    static int unnamed_value();

    template <typename T, std::enable_if_t<trait<T>::value, int> Dummy = 0>
    static int named_value();

    template <typename T, typename = std::enable_if_t<trait<T>::value>>
    static int type_gate();
};

template <typename T>
auto sfinae_return(T) -> std::enable_if_t<trait<T>::value, int>;

template <typename T>
int sfinae_param(T, std::enable_if_t<trait<T>::value, int>* = nullptr);

template <typename T, typename Enable = void>
struct specialization_gate;

template <typename T>
struct specialization_gate<T, std::enable_if_t<trait<T>::value>> {
    static constexpr int value = 4;
};

int main() {
    unnamed_type_default<int>::type value = 0;
    return unnamed_typed_nttp<int>::value +
           named_typed_nttp<int, 3>::value +
           specialization_gate<int>::value +
           value;
}
