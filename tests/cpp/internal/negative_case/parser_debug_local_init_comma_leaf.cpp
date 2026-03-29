// Parser-debug regression: keep the committed expression leaf for the
// repeated `stl_uninitialized.h` local-initializer comma family instead of
// rewriting it into a wrapper summary.

#define USE_ASSIGN_FOR_INIT(T, U) __is_trivial(T) && __is_assignable(T&, U)

template<typename T>
void probe(const T& value) {
    using ValueType = int;
    const bool can_fill = USE_ASSIGN_FOR_INIT(ValueType, const T&);
    (void)can_fill;
    (void)value;
}

