/* Stress inline diagnostics fallback behavior.  Always_inline functions that
   cannot be expanded (variadic + recursive) should degrade to calls cleanly
   even when invoked repeatedly. */

#include <stdarg.h>
#include <stddef.h>

static inline __attribute__((always_inline))
int diag_variadic_sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += va_arg(ap, int);
    }
    va_end(ap);
    return total;
}

static inline __attribute__((always_inline))
int diag_recursive(int depth) {
    if (depth <= 0) {
        return 0;
    }
    return depth + diag_recursive(depth - 1);
}

static inline __attribute__((always_inline))
int diag_recursive_tail(int depth, int base) {
    if (depth == 0) {
        return base;
    }
    return diag_recursive_tail(depth - 1, base + depth);
}

static inline __attribute__((always_inline))
int diag_variadic_stride(int stride, int count, ...) {
    va_list ap;
    va_start(ap, count);
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        (void)stride;
        sum += va_arg(ap, int);
    }
    va_end(ap);
    return sum;
}

static int stress_variadic(int rounds) {
    int acc = 0;
    for (int i = 0; i < rounds; ++i) {
        acc += diag_variadic_sum(3, i, i + 1, i + 2);
        acc ^= diag_variadic_stride(2, 2, i, i + 4);
    }
    return acc;
}

static int stress_recursive(int limit) {
    int acc = 0;
    for (int i = 0; i < limit; ++i) {
        acc += diag_recursive(i);
        acc ^= diag_recursive_tail(i, 1);
    }
    return acc;
}

int main(void) {
    const int rounds = 256;
    const int limit = 64;

    int expected_variadic = 0;
    for (int i = 0; i < rounds; ++i) {
        expected_variadic += (3 * i + 3);
        expected_variadic ^= (2 * i + 4);
    }
    if (stress_variadic(rounds) != expected_variadic) {
        return 1;
    }

    int expected_recursive = 0;
    for (int i = 0; i < limit; ++i) {
        const int tri = (i * (i + 1)) / 2;
        expected_recursive += tri;
        expected_recursive ^= (tri + 1);
    }
    if (stress_recursive(limit) != expected_recursive) {
        return 2;
    }

    return 0;
}
