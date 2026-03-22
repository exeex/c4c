// Test: EASTL bring-up slice 6 — template default value >> splitting, ref-qualified
// methods, using type aliases in struct/function body, sizeof...(pack), constexpr
// constructor detection, static_assert in struct body, (*const& ptr) declarator,
// and initializer <> tracking for comma protection.

// --- Fix 1: >> in struct-body template default values ---
template<bool B, typename T = void> struct enable_if {};
template<typename T> struct enable_if<true, T> { typedef T type; };
template<bool B, typename T = void> using enable_if_t = typename enable_if<B, T>::type;
template<typename T> struct is_val { static constexpr bool value = true; };

template<typename T1, typename T2>
struct Pair {
    T1 first;
    T2 second;

    // Triple > at end: is_val<TT2>>> closes three angles
    template <typename TT1 = T1,
              typename TT2 = T2,
              class = enable_if_t<is_val<TT2>::value>>
    constexpr Pair() : first(), second() {}

    // Nested template default with && in expression
    template <typename TT1 = T1,
              typename TT2 = T2,
              class = enable_if_t<is_val<TT1>::value && is_val<TT2>::value>>
    void test_method() {}
};

// --- Fix 2: ref-qualified methods (&, &&) ---
struct RefQual {
    int val;
    int get() & { return val; }
    int get() const & { return val; }
    int get() && { return val; }

    // Ref-qualified operator
    int operator()() & { return val; }
    int operator()() const & { return val; }
    int operator()() && { return val; }
};

// --- Fix 3: using type alias in function body ---
int test_using_in_function() {
    using MyInt = int;
    MyInt x = 42;
    return x;
}

// --- Fix 4: sizeof...(pack) ---
template <typename T, T... Ints>
struct IntSeq {
    static constexpr unsigned long size() { return sizeof...(Ints); }
};

// --- Fix 5: using type alias in struct body (registers as typedef) ---
struct WithUsing {
    using F = int (*)(int);
    // operator F() — conversion to function pointer, requires F as typedef
    operator F() const { return 0; }
};

// --- Fix 6: constexpr before constructor detection ---
struct ConstexprCtor {
    int x;
    constexpr ConstexprCtor() : x(0) {}
    constexpr ConstexprCtor(const ConstexprCtor& o) = default;
    constexpr ConstexprCtor(ConstexprCtor&& o) = default;
    constexpr ConstexprCtor& operator=(const ConstexprCtor&) = default;
};

// --- Fix 7: static_assert in struct body ---
template<typename T> struct is_integral { static constexpr bool value = true; };
template <typename T>
struct CheckedType {
    typedef T value_type;
    static_assert(is_integral<T>::value, "T must be integral");
};

// --- Fix 8: (*const& ptr) function pointer declarator ---
template <typename Result>
bool is_null(Result (*const& function_pointer)()) {
    return function_pointer == 0;
}

// --- Fix 9: initializer <> tracking protects comma ---
template<typename T, typename U> struct is_same { static constexpr bool value = true; };
template<typename T> struct is_copy { static constexpr bool value = true; };
template<typename A, typename B>
struct InitTrack {
    using vt = int;
    static constexpr bool value = is_same<A, B>::value &&
                                  is_copy<A>::value &&
                                  (is_same<A, int>::value || is_copy<B>::value);
};

// --- Fix 10: duplicate field names tolerated in C++ mode ---
template<typename T>
struct HasValue1 { static constexpr bool value = true; };
template<typename T>
struct HasValue2 { static constexpr bool value = false; };

int main() {
    int result = 0;
    result += test_using_in_function();

    return result - 42;  // should return 0
}
