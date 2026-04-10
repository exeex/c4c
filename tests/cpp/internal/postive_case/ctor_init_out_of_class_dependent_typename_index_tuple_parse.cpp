// Parse-only regression: an out-of-class constructor template definition
// should accept dependent `typename X<...>::__type()` temporaries inside a
// delegating constructor initializer list.
// RUN: %c4cll --parse-only %s

namespace std {

struct piecewise_construct_t {};

template <typename... Ts>
struct tuple {};

template <unsigned long... Is>
struct _Index_tuple {};

template <unsigned long N>
struct _Build_index_tuple {
    using __type = _Index_tuple<>;
};

template <typename T1, typename T2>
struct pair {
    template <class... Args1, class... Args2>
    pair(piecewise_construct_t, tuple<Args1...> __first, tuple<Args2...> __second);

    template <class... Args1, class... Args2, unsigned long... I1, unsigned long... I2>
    pair(tuple<Args1...>, tuple<Args2...>, _Index_tuple<I1...>, _Index_tuple<I2...>) {}
};

template <class T1, class T2>
template <typename... Args1, typename... Args2>
inline pair<T1, T2>::pair(piecewise_construct_t,
                          tuple<Args1...> __first,
                          tuple<Args2...> __second)
    : pair(__first,
           __second,
           typename _Build_index_tuple<sizeof...(Args1)>::__type(),
           typename _Build_index_tuple<sizeof...(Args2)>::__type()) {}

}  // namespace std

int main() {
    return 0;
}
