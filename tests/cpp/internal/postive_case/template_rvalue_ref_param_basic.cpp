// Tests template function with T&& parameter — basic rvalue ref pass-through.
// Phase 4 of rvalue_ref_plan.md.

// Template move helper
template <typename T>
T&& my_move(T& x) {
    return static_cast<T&&>(x);
}

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

int main() {
    // Basic: move helper works with scalar types
    int x = 42;
    int&& r = my_move(x);
    if (r != 42) return 1;

    // Move helper with struct triggers move constructor
    Obj a(100);
    Obj b(my_move(a));
    if (b.value != 100) return 2;
    if (b.how != 2) return 3;      // must be move-constructed
    if (a.value != -1) return 4;   // source marked moved-from

    // Chain: move twice
    Obj c(200);
    Obj d(my_move(c));
    Obj e(my_move(d));
    if (e.value != 200) return 5;
    if (e.how != 2) return 6;
    if (d.value != -1) return 7;

    return 0;
}
