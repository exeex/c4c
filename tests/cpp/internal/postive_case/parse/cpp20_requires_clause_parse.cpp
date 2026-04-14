// Reduced from libstdc++ C++20 headers: a templated declaration can carry a
// requires-clause that includes both boolean constraint expressions and a
// nested requires-expression block.
// RUN: %c4cll --parse-only %s

template<typename T>
struct trait {
    static constexpr bool value = false;
};

template<typename T>
requires (!trait<T>::value) && requires (T& t) { t.~T(); }
inline constexpr bool can_destroy = true;
