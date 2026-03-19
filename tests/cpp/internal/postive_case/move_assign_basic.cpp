// Basic move assignment operator test.
// Tests: operator=(ClassName&&) declaration, invocation via static_cast.
struct Obj {
    int value;
    int assigned;  // 1 if this object was move-assigned

    Obj(int v) {
        value = v;
        assigned = 0;
    }

    Obj& operator=(Obj&& other) {
        value = other.value;
        assigned = 1;
        other.value = -1;
        return *this;
    }

    int get() const { return value; }
    int was_assigned() const { return assigned; }
};

int main() {
    Obj a(42);
    Obj b(0);

    if (a.get() != 42) return 1;
    if (b.get() != 0) return 2;

    // Move assign b from a
    b = static_cast<Obj&&>(a);
    if (b.get() != 42) return 3;
    if (b.was_assigned() != 1) return 4;
    if (a.get() != -1) return 5;

    return 0;
}
