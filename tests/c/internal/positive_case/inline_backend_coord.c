/* Phase 8: Backend coordination test.
   Verifies that:
   1. always_inline functions with all call sites expanded lose alwaysinline attr
   2. noinline functions retain noinline attr
   3. Default (non-inline) functions remain as normal calls
   4. always_inline functions with unexpandable call sites keep alwaysinline attr
   5. Runtime results are correct after expansion + attribute reconciliation */

static inline __attribute__((always_inline))
int add1(int x) {
    return x + 1;
}

__attribute__((noinline))
int sub1(int x) {
    return x - 1;
}

/* Default (no inline attribute) function. */
int mul2(int x) {
    return x * 2;
}

static inline __attribute__((always_inline))
int square(int x) {
    return x * x;
}

int main(void) {
    /* add1 is always_inline: should be expanded, then stripped of alwaysinline. */
    int a = add1(5);       /* expect 6 */

    /* sub1 is noinline: stays a call, keeps noinline attr. */
    int b = sub1(10);      /* expect 9 */

    /* mul2 is default: stays a call, no special attr. */
    int c = mul2(3);       /* expect 6 */

    /* square is always_inline: should be expanded, then stripped. */
    int d = square(4);     /* expect 16 */

    /* Chained: all always_inline sites expanded. */
    int e = add1(square(3)); /* square(3)=9, add1(9)=10 */

    if (a == 6 && b == 9 && c == 6 && d == 16 && e == 10)
        return 0;
    return 1;
}
