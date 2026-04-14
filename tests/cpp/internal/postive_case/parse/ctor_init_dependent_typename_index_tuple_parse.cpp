// Parse-only regression: constructor initializer arguments should accept
// dependent `typename X<...>::__type()` functional-cast-style temporaries in a
// delegating constructor.
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

template <typename T>
struct pair {
    T first;
    T second;

    template <class... Args1, class... Args2>
    pair(piecewise_construct_t,
         tuple<Args1...> __first,
         tuple<Args2...> __second)
        : pair(__first,
               __second,
               typename _Build_index_tuple<sizeof...(Args1)>::__type(),
               typename _Build_index_tuple<sizeof...(Args2)>::__type()) {}

    template <class... Args1, class... Args2, unsigned long... I1, unsigned long... I2>
    pair(tuple<Args1...>,
         tuple<Args2...>,
         _Index_tuple<I1...>,
         _Index_tuple<I2...>) {}
};

}  // namespace std

int main() {
    return 0;
}
