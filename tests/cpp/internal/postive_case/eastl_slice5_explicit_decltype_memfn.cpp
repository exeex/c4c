// Test: EASTL bring-up slice 5 — explicit keyword, conversion operator with
// inner typedef, free operator<= with template args, qualified param types,
// template member access in expressions, explicit constructors, decltype skip,
// pointer-to-member-function params, and constexpr initializer with < operator.

// --- Error 1: explicit keyword before operator ---
struct ExplicitConv {
    explicit operator bool() const;
    explicit operator int() const;
};

// --- Error 2: operator value_type() where value_type is inner typedef ---
template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    typedef T value_type;
    typedef integral_constant<T, v> type;

    constexpr operator value_type() const { return value; }
    constexpr value_type operator()() const { return value; }
};

typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;

// --- Error 3: free operator<= with template args in params ---
template <typename T>
struct Wrap {
    T val;
};

template <typename T1, typename T2>
inline bool operator<=(const Wrap<T1>& a, const Wrap<T2>& b)
    { return true; }

// --- Error 4-5: qualified type in function params (ns::tag) ---
namespace eastl_ns {
    struct input_tag {};
    struct bidir_tag {};
}

template <typename Iter>
inline int distance_impl(Iter first, Iter last, eastl_ns::input_tag)
{
    return 0;
}

template <typename Iter>
inline void advance_impl(Iter& i, int n, eastl_ns::bidir_tag)
{
}

// --- Error 6: template<type,type>::value in expression ---
template <typename A, typename B>
struct can_memmove {
    static constexpr bool value = false;
};

template <typename In, typename Out>
inline Out copy_chooser(In first, In last, Out result)
{
    const bool ok = can_memmove<In, Out>::value;
    return result;
}

// --- Error 7: explicit before constructors in struct body ---
struct MoveIter {
    typedef int iterator_type;
    explicit MoveIter(iterator_type mi) {}
    explicit MoveIter() {}
};

// --- Error 8: decltype with complex expressions (C++ mode: skip balanced) ---
template <typename F>
inline decltype(sizeof(int)) invoke_result(F&& func)
{
    return 0;
}

// --- Error 9: pointer-to-member-function params ---
template <typename R, typename T>
struct mem_fn_impl {};

template <typename R, typename T>
inline mem_fn_impl<R (T::*)()> mem_fn_test(R (T::*pm)())
{ return mem_fn_impl<R (T::*)()>(); }

template <typename R, typename T>
inline mem_fn_impl<R (T::*)() const> mem_fn_const(R (T::*pm)() const)
{ return mem_fn_impl<R (T::*)() const>(); }

// --- Error 10: constexpr initializer with < comparison (not template) ---
template<>
struct integral_constant<int, 0> {
    typedef int value_type;
    static constexpr int  digits = ((((value_type)(-1) < 0) ? 7 : 8));
    static constexpr bool is_signed = ((value_type)(-1) < 0);
};

int main() {
    return 0;
}
