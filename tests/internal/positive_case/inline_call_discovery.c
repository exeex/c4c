/* Test that always_inline functions are called correctly.
   Phase 1: call-site discovery identifies these as inline candidates.
   Phase 2+: these calls will be expanded inline.
   For now, they compile and run as normal calls. */

static inline __attribute__((always_inline))
int add_one(int x) {
    return x + 1;
}

static inline __attribute__((always_inline))
int square(int x) {
    return x * x;
}

__attribute__((noinline))
int noinline_sub(int a, int b) {
    return a - b;
}

int main(void) {
    /* always_inline candidates */
    int a = add_one(5);       /* expect 6 */
    int b = square(4);        /* expect 16 */

    /* noinline — should NOT be an inline candidate */
    int c = noinline_sub(10, 3); /* expect 7 */

    /* combined */
    int d = add_one(square(3));  /* expect 10 */

    if (a != 6) return 1;
    if (b != 16) return 2;
    if (c != 7) return 3;
    if (d != 10) return 4;
    return 0;
}
