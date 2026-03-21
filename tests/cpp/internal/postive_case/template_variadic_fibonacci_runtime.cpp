// Runtime coverage for variadic function templates.
// This checks that a variadic template can be instantiated and executed
// in the runtime pipeline while computing a Fibonacci result.

constexpr int fib_plain(int n) {
    if (n <= 1) return n;
    return fib_plain(n - 1) + fib_plain(n - 2);
}

template <typename... Ts>
int fib_runtime(int n) {
    return fib_plain(n);
}

int main() {
    int fib9 = fib_runtime<int, char, long>(9);
    int fib10 = fib_runtime<unsigned, short>(10);

    if (fib9 != 34) return 1;
    if (fib10 != 55) return 2;
    return 0;
}
