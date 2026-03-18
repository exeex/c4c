/* Test Phase 6: inline expansion in expression contexts.
   Verifies that always_inline calls nested inside larger expressions
   are correctly hoisted and expanded:
   - call as operand of binary expression
   - multiple calls in one expression
   - call inside assignment RHS (not init)
   - call as argument to another (non-inline) call
   - call inside ternary condition / operands
   - call inside cast expression */

static inline __attribute__((always_inline))
int square(int x) {
    return x * x;
}

static inline __attribute__((always_inline))
int add(int a, int b) {
    return a + b;
}

static inline __attribute__((always_inline))
int negate(int x) {
    return -x;
}

static inline __attribute__((always_inline))
int identity(int x) {
    return x;
}

static int passthrough(int x) {
    return x;
}

int main(void) {
    /* 1. Call as operand of binary expression: square(3) + 1 = 10 */
    int a = square(3) + 1;
    if (a != 10) return 1;

    /* 2. Two calls in one binary expression: add(2,3) + square(4) = 5+16=21 */
    int b = add(2, 3) + square(4);
    if (b != 21) return 2;

    /* 3. Call inside assignment RHS (not initializer) */
    int c = 0;
    c = square(5);
    if (c != 25) return 3;

    /* 4. Nested expression: square(3) * add(1,2) = 9*3 = 27 */
    int d = square(3) * add(1, 2);
    if (d != 27) return 4;

    /* 5. Call as argument to non-inline function */
    int e = passthrough(square(6));
    if (e != 36) return 5;

    /* 6. Call inside ternary */
    int f = (square(2) > 3) ? add(10, 20) : 0;
    if (f != 30) return 6;

    /* 7. Call inside cast */
    long g = (long)square(7);
    if (g != 49) return 7;

    /* 8. Call result used in comparison */
    if (add(3, 4) != 7) return 8;

    /* 9. Multiple calls in return expression */
    int h = add(square(2), square(3));  /* add(4, 9) = 13 */
    if (h != 13) return 9;

    /* 10. Unary minus on inline call */
    int i = -negate(5);  /* -(-5) = 5 */
    if (i != 5) return 10;

    /* 11. Complex chain: add(square(2), add(1, square(3))) = add(4, add(1,9)) = add(4,10) = 14 */
    int j = add(square(2), add(1, square(3)));
    if (j != 14) return 11;

    return 0;
}
