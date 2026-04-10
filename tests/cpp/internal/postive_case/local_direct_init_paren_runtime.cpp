// Runtime regression: block-scope direct-initialization declarations with
// parentheses should produce local variables rather than empty statements.
int compute(int x) {
    int i(0);
    const bool positive(x > 0);
    if (positive)
        i = x;
    return i;
}

int main() {
    if (compute(7) != 7) return 1;
    if (compute(-3) != 0) return 2;
    return 0;
}
