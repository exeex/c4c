/* Phase 7: Inline diagnostics and unsupported cases.
   Tests that noinline suppresses expansion, default stays a call,
   and unsupported always_inline cases (variadic, self-recursive) fall
   back to normal calls with correct runtime behavior (permissive mode). */

/* --- noinline always suppresses --- */
__attribute__((noinline))
int noinline_add(int x, int y) {
    return x + y;
}

/* --- default (no inline annotation) stays a call --- */
int default_mul(int x, int y) {
    return x * y;
}

/* --- self-recursive always_inline: cannot expand, falls back to call --- */
static inline __attribute__((always_inline))
int recursive_fib(int n) {
    if (n <= 1) return n;
    return recursive_fib(n - 1) + recursive_fib(n - 2);
}

/* --- variadic always_inline: cannot expand, falls back to call --- */
#include <stdarg.h>
static inline __attribute__((always_inline))
int variadic_sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int sum = 0;
    for (int i = 0; i < count; i++)
        sum += va_arg(ap, int);
    va_end(ap);
    return sum;
}

/* --- noinline wins over inline keyword --- */
__attribute__((noinline)) static inline
int noinline_wins(int x) {
    return x + 100;
}

/* --- normal always_inline (should expand successfully) --- */
static inline __attribute__((always_inline))
int simple_inc(int x) {
    return x + 1;
}

int main(void) {
    /* noinline: correct call */
    if (noinline_add(3, 4) != 7) return 1;

    /* default: correct call */
    if (default_mul(3, 5) != 15) return 2;

    /* self-recursive always_inline: falls back to call */
    if (recursive_fib(10) != 55) return 3;

    /* variadic always_inline: falls back to call */
    if (variadic_sum(3, 10, 20, 30) != 60) return 4;

    /* noinline+inline: noinline wins, correct call */
    if (noinline_wins(5) != 105) return 5;

    /* simple always_inline: should expand */
    if (simple_inc(41) != 42) return 6;

    return 0;
}
