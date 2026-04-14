// Test: template struct specialization parsing (explicit and partial).
// RUN: %c4cll --parse-only %s
struct true_type { static const bool value = 1; };
struct false_type { static const bool value = 0; };

// Primary with body + explicit specializations
template <typename T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};
template <> struct is_void<const void> : true_type {};
template <> struct is_void<volatile void> : true_type {};
template <> struct is_void<const volatile void> : true_type {};

// Forward-decl-only primary + partial specializations
template <int I0, int... In>
struct static_min;

template <int I0>
struct static_min<I0>
    { static const int value = I0; };

// Type param partial specialization
template <typename T> struct remove_const { typedef T type; };
template <typename T> struct remove_const<const T> { typedef T type; };

// Variadic partial specializations
template <class...>
struct disjunction : false_type {};
template <class B>
struct disjunction<B> : B {};
template <class B, class... Bn>
struct disjunction<B, Bn...> : B {};

// NTTP default with complex expression (>:: continuation)
template <typename T, bool = is_void<T>::value || is_void<T>::value>
struct complex_default
    { static const int val = 0; };
template <typename T>
struct complex_default<T, true>
    { static const int val = 1; };

int main() {
    return 0;
}
