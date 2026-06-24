// Negative test: inline asm templates must fold to accepted literal text before
// lowering; runtime template expressions are rejected instead of being dropped.

void f(const char* templ, int value) {
    __asm__ __volatile__(templ : "+r"(value) : : "memory");
}
