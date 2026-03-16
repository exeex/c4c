/* Test Phase 4: minimal CFG splice for single-return inline expansion.
   Verifies that:
   - simple always_inline functions are expanded at call sites
   - return values are correctly captured
   - void callees are inlined correctly
   - arguments are evaluated exactly once
   - control flow after the inlined body continues normally */

static int side_effect_counter;

static inline __attribute__((always_inline))
int add1(int x) {
    return x + 1;
}

static inline __attribute__((always_inline))
int add(int a, int b) {
    return a + b;
}

static inline __attribute__((always_inline))
void inc_counter(void) {
    side_effect_counter++;
}

static inline __attribute__((always_inline))
int mul2(int x) {
    return x * 2;
}

static int next_val(void) {
    side_effect_counter++;
    return side_effect_counter;
}

int main(void) {
    /* simple return-value capture in local init */
    int a = add1(5);
    if (a != 6) return 1;

    /* two-parameter inline */
    int b = add(10, 20);
    if (b != 30) return 2;

    /* void callee in ExprStmt */
    side_effect_counter = 0;
    inc_counter();
    if (side_effect_counter != 1) return 3;

    /* argument with side effects: evaluated exactly once */
    side_effect_counter = 0;
    int c = mul2(next_val());  /* next_val returns 1 */
    if (c != 2) return 4;
    if (side_effect_counter != 1) return 5;

    /* chained: result of one inline feeds another */
    int d = add1(add1(10));  /* 10+1=11, 11+1=12 */
    if (d != 12) return 6;

    /* inline in sequence: code after inline site executes */
    int e = 100;
    int f = add(e, add1(0));  /* add(100, 1) = 101 */
    if (f != 101) return 7;

    return 0;
}
