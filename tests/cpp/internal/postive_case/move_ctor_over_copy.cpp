// Tests that move constructor is preferred over copy constructor for rvalue args.
// Also tests copy constructor is used for lvalue args.
struct Obj {
    int value;
    int how;  // 0=default-ctor, 1=copy-ctor, 2=move-ctor

    Obj(int v) {
        value = v;
        how = 0;
    }

    // Copy constructor
    Obj(const Obj& other) {
        value = other.value;
        how = 1;
    }

    // Move constructor
    Obj(Obj&& other) {
        value = other.value;
        how = 2;
        other.value = -1;
    }

    int get() const { return value; }
    int get_how() const { return how; }
};

int main() {
    Obj a(42);
    if (a.get() != 42) return 1;
    if (a.get_how() != 0) return 2;

    // Copy construct from lvalue — should use copy ctor
    Obj b(a);
    if (b.get() != 42) return 3;
    if (b.get_how() != 1) return 4;
    if (a.get() != 42) return 5;  // copy should not modify source

    // Move construct from rvalue — should use move ctor
    Obj c(static_cast<Obj&&>(a));
    if (c.get() != 42) return 6;
    if (c.get_how() != 2) return 7;
    if (a.get() != -1) return 8;  // move should mark source

    return 0;
}
