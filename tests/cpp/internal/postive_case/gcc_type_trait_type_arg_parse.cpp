// Parse-only regression: GCC-style type-trait builtins accept type-name
// argument lists such as `__is_assignable(T&, U)` inside local initializers.
// RUN: %c4cll --parse-only %s

template <typename T>
int probe(const T& value) {
    using ValueType = int;
    const bool can_fill = __is_trivial(ValueType) && __is_assignable(ValueType&, const T&);
    return can_fill ? 1 : 0;
}

int main() {
    return probe(0);
}
