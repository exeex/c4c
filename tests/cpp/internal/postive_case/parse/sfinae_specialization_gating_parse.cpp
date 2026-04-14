// Parse regression: isolate SFINAE-shaped partial-specialization gating so the
// parser accepts the specialization head independently of later semantics.

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

template <typename T, typename Enable = void>
struct specialization_gate;

template <typename T>
struct specialization_gate<T, std::enable_if_t<trait<T>::value>> {
    using type = T;
    static constexpr int value = 7;
};

using gated_int = specialization_gate<int>::type;

int main() {
    gated_int value = 0;
    return specialization_gate<int>::value + value;
}
