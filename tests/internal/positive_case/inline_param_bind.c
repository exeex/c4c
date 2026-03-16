/* Test Phase 3: argument capture and parameter binding for inline expansion.
   Verifies that:
   - arguments are evaluated exactly once (no duplication of side effects)
   - multiple parameter references read the same captured value
   - argument order is preserved
   - void callees work correctly */

static int call_count;

static inline __attribute__((always_inline))
int add(int a, int b) {
    return a + b;
}

static inline __attribute__((always_inline))
int use_param_twice(int x) {
    return x + x;  /* parameter referenced twice: must not evaluate arg twice */
}

static inline __attribute__((always_inline))
void set_count(int val) {
    call_count = val;
}

static int next_val(void) {
    call_count++;
    return call_count;
}

int main(void) {
    /* basic two-param inline */
    int a = add(3, 4);
    if (a != 7) return 1;

    /* parameter used twice: arg must be captured once */
    call_count = 0;
    int b = use_param_twice(next_val()); /* next_val returns 1, call_count becomes 1 */
    if (b != 2) return 2;
    if (call_count != 1) return 3;  /* next_val should have been called exactly once */

    /* void callee */
    set_count(42);
    if (call_count != 42) return 4;

    /* nested inline call: add(add(1,2), add(3,4)) */
    int c = add(add(1, 2), add(3, 4));
    if (c != 10) return 5;

    return 0;
}
