// Runtime regression: non-dependent `__is_unsigned(T)` should evaluate to the
// correct frontend constant for basic signed and unsigned integer types.

static_assert(__is_unsigned(unsigned));
static_assert(!__is_unsigned(int));

int main() {
    if ((__is_unsigned(unsigned) ? 1 : 0) != 1)
        return 1;
    if ((__is_unsigned(int) ? 1 : 0) != 0)
        return 2;
    return 0;
}
