/* Test __attribute__((noinline)) and __attribute__((always_inline)) parsing
   and LLVM IR attribute emission. */

__attribute__((noinline))
int noinline_add(int a, int b) {
    return a + b;
}

static inline __attribute__((always_inline))
int always_inline_mul(int a, int b) {
    return a * b;
}

int main(void) {
    int sum = noinline_add(3, 4);
    int prod = always_inline_mul(5, 6);
    if (sum == 7 && prod == 30) return 0;
    return 1;
}
