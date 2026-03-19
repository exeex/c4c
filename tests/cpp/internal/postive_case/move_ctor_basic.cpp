// Basic move constructor test.
// Tests: ClassName(ClassName&& other) declaration, invocation via static_cast.
struct Obj {
    int value;
    int moved;  // 1 if this object was move-constructed

    Obj(int v) {
        value = v;
        moved = 0;
    }

    Obj(Obj&& other) {
        value = other.value;
        moved = 1;
        other.value = -1;  // mark source as moved-from
    }

    int get() const { return value; }
    int was_moved() const { return moved; }
};

int main() {
    Obj a(42);
    if (a.get() != 42) return 1;
    if (a.was_moved() != 0) return 2;

    // Move construct b from a
    Obj b(static_cast<Obj&&>(a));
    if (b.get() != 42) return 3;
    if (b.was_moved() != 1) return 4;

    // Source should be moved-from
    if (a.get() != -1) return 5;

    return 0;
}
