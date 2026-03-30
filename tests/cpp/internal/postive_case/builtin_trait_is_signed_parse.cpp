// Parse-only regression: Clang-style builtin trait `__is_signed(T)` should be
// accepted directly in expression context.
// RUN: %c4cll --parse-only %s

template <typename T>
int probe() {
    constexpr bool direct = __is_signed(T);
    constexpr bool pointer_case = __is_signed(T*);
    return direct || pointer_case ? 1 : 0;
}

int main() {
    return probe<int>();
}
