// Parse-only regression: Clang-style builtin trait `__is_unsigned(T)` should
// be accepted directly in expression context instead of relying only on
// indirect libc++ header coverage.
// RUN: %c4cll --parse-only %s

template <typename T>
int probe() {
    constexpr bool direct = __is_unsigned(T);
    constexpr bool pointer_case = __is_unsigned(T*);
    return direct || pointer_case ? 1 : 0;
}

int main() {
    return probe<unsigned>();
}
