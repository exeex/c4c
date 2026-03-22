// Test: EASTL bring-up slice 7 — unnamed template-id parameters in a
// constructor parameter list should parse cleanly and not corrupt following
// methods in the same struct body.

namespace eastl {

struct piecewise_construct_t {};

template <typename... Ts>
struct tuple {};

template <unsigned long I, typename Tuple>
struct tuple_element;

template <unsigned long I, typename Tuple>
using tuple_element_t = int;

template <typename T>
struct is_lvalue_reference {
    static constexpr bool value = true;
};

template <bool B, typename T, typename F>
struct conditional {
    using type = T;
};

template <typename T>
struct add_lvalue_reference {
    using type = T&;
};

template <typename T>
struct remove_reference {
    using type = T;
};

template <unsigned long I, typename Tuple>
using const_tuple_element_t = typename conditional<
    is_lvalue_reference<tuple_element_t<I, Tuple>>::value,
    typename add_lvalue_reference<const typename remove_reference<tuple_element_t<I, Tuple>>::type>::type,
    const tuple_element_t<I, Tuple>>::type;

template <unsigned long I, typename... Ts>
tuple_element_t<I, tuple<Ts...>>& indexed_get(tuple<Ts...>& t);

template <unsigned long I, typename... Ts>
const_tuple_element_t<I, tuple<Ts...>>& const_indexed_get(const tuple<Ts...>& t);

template <typename T, typename... Ts>
T& typed_get(tuple<Ts...>& t);

template <unsigned long... Is>
struct index_sequence {};

template <unsigned long N>
using make_index_sequence = index_sequence<>;

template <typename T>
T&& forward(T&);

template <unsigned long I, typename... Ts>
int get(tuple<Ts...>&);

template <typename T>
void iter_swap(T*, T*) {}

template <typename T>
struct pair {
    T first;
    T second;

    template <class... Args1, class... Args2>
    pair(piecewise_construct_t pwc,
         tuple<Args1...> first_args,
         tuple<Args2...> second_args)
        : pair(pwc,
               first_args,
               second_args,
               make_index_sequence<sizeof...(Args1)>(),
               make_index_sequence<sizeof...(Args2)>()) {}

    template <class... Args1, class... Args2, unsigned long... I1, unsigned long... I2>
    pair(piecewise_construct_t,
         tuple<Args1...> first_args,
         tuple<Args2...> second_args,
         index_sequence<I1...>,
         index_sequence<I2...>)
        : first(forward<Args1>(get<I1>(first_args))...)
        , second(forward<Args2>(get<I2>(second_args))...) {}

    void swap(pair& p) {
        iter_swap(&first, &p.first);
        iter_swap(&second, &p.second);
    }
};

}  // namespace eastl

int main() {
    return 0;
}
