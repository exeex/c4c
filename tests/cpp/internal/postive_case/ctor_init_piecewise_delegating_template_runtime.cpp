// HIR regression: instantiated template constructors must preserve enough
// source-name metadata for piecewise delegating constructor helpers to keep
// resolving as self-delegation instead of falling back to scalar member init.

namespace eastl {

struct piecewise_construct_t {};
constexpr piecewise_construct_t piecewise_construct{};

template <typename T>
T&& move(T& t) { return static_cast<T&&>(t); }

template <typename T>
T&& forward(T& t) { return static_cast<T&&>(t); }

template <unsigned long... Is>
struct index_sequence {};

template <unsigned long N>
using make_index_sequence = index_sequence<0>;

template <typename T>
struct tuple {
    T value;
};

template <unsigned long I, typename T>
T& get(tuple<T>& t) {
    return t.value;
}

template <typename T1, typename T2>
struct pair {
    T1 first;
    T2 second;

    template <class... Args1, class... Args2>
    pair(piecewise_construct_t pwc, tuple<Args1...> first_args, tuple<Args2...> second_args)
        : pair(pwc,
               move(first_args),
               move(second_args),
               make_index_sequence<sizeof...(Args1)>(),
               make_index_sequence<sizeof...(Args2)>()) {}

private:
    template <class... Args1, class... Args2, unsigned long... I1, unsigned long... I2>
    pair(piecewise_construct_t,
         tuple<Args1...> first_args,
         tuple<Args2...> second_args,
         index_sequence<I1...>,
         index_sequence<I2...>)
        : first(forward<Args1>(get<I1>(first_args))...)
        , second(forward<Args2>(get<I2>(second_args))...) {}
};

}  // namespace eastl

int main() {
    eastl::tuple<int> a{4};
    eastl::tuple<int> b{9};
    auto p = eastl::pair<int, int>(eastl::piecewise_construct, a, b);
    return (p.first == 4 && p.second == 9) ? 0 : 1;
}
