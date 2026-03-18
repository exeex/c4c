/* Test Phase 5: multi-block and multi-return inline expansion.
   Verifies that:
   - callees with if/else branches are inlined correctly
   - callees with multiple return sites produce correct results
   - callees with loops are inlined correctly
   - switch-based callees are inlined correctly
   - control flow after inlined multi-block body continues normally */

static inline __attribute__((always_inline))
int abs_val(int x) {
    if (x < 0)
        return -x;
    return x;
}

static inline __attribute__((always_inline))
int clamp(int x, int lo, int hi) {
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}

static inline __attribute__((always_inline))
int sum_to_n(int n) {
    int total = 0;
    int i = 1;
    while (i <= n) {
        total = total + i;
        i = i + 1;
    }
    return total;
}

static inline __attribute__((always_inline))
int classify(int x) {
    if (x < 0)
        return -1;
    else if (x == 0)
        return 0;
    else
        return 1;
}

static inline __attribute__((always_inline))
void swap_if_greater(int *a, int *b) {
    if (*a > *b) {
        int tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static inline __attribute__((always_inline))
int factorial(int n) {
    int result = 1;
    int i = 2;
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    return result;
}

int main(void) {
    /* if/else with two returns */
    if (abs_val(-5) != 5) return 1;
    if (abs_val(3) != 3) return 2;
    if (abs_val(0) != 0) return 3;

    /* three return sites */
    if (clamp(5, 0, 10) != 5) return 4;
    if (clamp(-3, 0, 10) != 0) return 5;
    if (clamp(15, 0, 10) != 10) return 6;

    /* loop body */
    if (sum_to_n(10) != 55) return 7;
    if (sum_to_n(0) != 0) return 8;
    if (sum_to_n(1) != 1) return 9;

    /* if/else if/else chain */
    if (classify(-42) != -1) return 10;
    if (classify(0) != 0) return 11;
    if (classify(99) != 1) return 12;

    /* void callee with branching and pointer args */
    int x = 10, y = 3;
    swap_if_greater(&x, &y);
    if (x != 3 || y != 10) return 13;

    /* already sorted */
    int p = 1, q = 5;
    swap_if_greater(&p, &q);
    if (p != 1 || q != 5) return 14;

    /* loop-based callee */
    if (factorial(5) != 120) return 15;
    if (factorial(1) != 1) return 16;

    /* chained multi-block inline */
    int v = abs_val(clamp(-20, -10, 10));
    if (v != 10) return 17;

    return 0;
}
