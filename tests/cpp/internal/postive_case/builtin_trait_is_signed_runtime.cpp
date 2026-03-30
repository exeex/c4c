// Runtime regression: non-dependent `__is_signed(T)` should evaluate to the
// correct frontend constant for basic signed and unsigned integer types.

static_assert(__is_signed(int));
static_assert(!__is_signed(unsigned));

int main() {
    if ((__is_signed(int) ? 1 : 0) != 1)
        return 1;
    if ((__is_signed(unsigned) ? 1 : 0) != 0)
        return 2;
    return 0;
}
