// Basic constructor declaration and invocation test.
// Tests: ClassName(params) constructor declaration in struct body,
//        ClassName var(args) direct construction syntax.
struct Box {
    int value;

    Box(int v) {
        value = v;
    }

    int get() const {
        return value;
    }
};

int main() {
    Box b(42);
    if (b.get() != 42) return 1;

    Box c(100);
    if (c.get() != 100) return 2;

    // Default member access still works
    if (b.value != 42) return 3;

    return 0;
}
