// Copy vs move assignment operator overload test.
// Tests: operator=(const T&) vs operator=(T&&) coexist, lvalue selects copy, rvalue selects move.
struct Obj {
    int value;
    int last_op;  // 1 = copy-assigned, 2 = move-assigned

    Obj(int v) {
        value = v;
        last_op = 0;
    }

    Obj& operator=(const Obj& other) {
        value = other.value;
        last_op = 1;
        return *this;
    }

    Obj& operator=(Obj&& other) {
        value = other.value;
        last_op = 2;
        other.value = -1;
        return *this;
    }

    int get() const { return value; }
    int op() const { return last_op; }
};

int main() {
    Obj a(10);
    Obj b(20);
    Obj c(30);

    // lvalue → copy assignment
    c = a;
    if (c.get() != 10) return 1;
    if (c.op() != 1) return 2;
    if (a.get() != 10) return 3;  // a unchanged

    // rvalue → move assignment
    c = static_cast<Obj&&>(b);
    if (c.get() != 20) return 4;
    if (c.op() != 2) return 5;
    if (b.get() != -1) return 6;  // b moved-from

    return 0;
}
