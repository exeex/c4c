// Parse-only regression: nested variable-template ids in non-type template
// arguments inside record base clauses must still be accepted when the next
// token is an enclosing template close.
// RUN: %c4cll --parse-only %s

namespace eastl {

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    typedef T value_type;
    typedef integral_constant<T, v> type;
};

template <typename T>
struct is_integral : integral_constant<bool, false> {};

template <>
struct is_integral<int> : integral_constant<bool, true> {};

template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;

template <typename T>
struct remove_cv {
    typedef T type;
};

template <typename T>
using remove_cv_t = typename remove_cv<T>::type;

template <typename T>
struct remove_all_extents {
    typedef T type;
};

template <typename T>
using remove_all_extents_t = typename remove_all_extents<T>::type;

template <typename T>
struct has_unique_object_representations
    : public integral_constant<bool,
                               is_integral_v<remove_cv_t<remove_all_extents_t<T>>>> {
};

}  // namespace eastl

int main() {
    return eastl::has_unique_object_representations<int>::value ? 0 : 1;
}
