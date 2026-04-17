// Runtime regression: block-scope direct-init with a single identifier argument
// should remain a variable declaration, not a function declaration.
struct Box {
    int value;
    Box(int v) : value(v) {}
};

int compute(int source) {
    Box copy(source);
    return copy.value;
}

int main() {
    return compute(11) == 11 ? 0 : 1;
}
