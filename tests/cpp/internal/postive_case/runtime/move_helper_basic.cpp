// Tests a template move() helper: static_cast<T&&>(x) inside a template function.
// Phase 4 of rvalue_ref_plan.md.

struct Obj {
    int value;
    int how;  // 0=normal, 1=copy, 2=move

    Obj(int v) {
        value = v;
        how = 0;
    }

    Obj(const Obj& other) {
        value = other.value;
        how = 1;
    }

    Obj(Obj&& other) {
        value = other.value;
        how = 2;
        other.value = -1;
    }
};

// Minimal move() helper — takes lvalue ref, returns rvalue ref via static_cast.
template <typename T>
T&& my_move(T& x) {
    return static_cast<T&&>(x);
}

int main() {
    Obj a(42);
    if (a.value != 42) return 1;
    if (a.how != 0) return 2;

    // Use my_move to trigger move constructor
    Obj b(my_move(a));
    if (b.value != 42) return 3;
    if (b.how != 2) return 4;      // must be move-constructed
    if (a.value != -1) return 5;   // source must be marked moved-from

    // Move again from a fresh object
    Obj c(10);
    Obj d(my_move(c));
    if (d.value != 10) return 6;
    if (d.how != 2) return 7;
    if (c.value != -1) return 8;

    return 0;
}
