// Basic copy assignment operator test.
// Tests: operator=(const ClassName&) declaration, invocation with lvalue.
struct Obj {
    int value;
    int copied;  // 1 if this object was copy-assigned

    Obj(int v) {
        value = v;
        copied = 0;
    }

    Obj& operator=(const Obj& other) {
        value = other.value;
        copied = 1;
        return *this;
    }

    int get() const { return value; }
    int was_copied() const { return copied; }
};

int main() {
    Obj a(42);
    Obj b(0);

    if (a.get() != 42) return 1;
    if (b.get() != 0) return 2;

    // Copy assign b from a (lvalue)
    b = a;
    if (b.get() != 42) return 3;
    if (b.was_copied() != 1) return 4;
    // a should be unchanged (copy, not move)
    if (a.get() != 42) return 5;

    // Copy assign from a different value
    Obj c(99);
    b = c;
    if (b.get() != 99) return 6;
    if (b.was_copied() != 1) return 7;

    return 0;
}
