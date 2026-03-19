// Tests copy initialization with both copy and move constructors.
// T b = a; (lvalue) should call copy ctor.
// T c = static_cast<T&&>(a); (rvalue) should call move ctor.
struct Obj {
    int value;
    int how;  // 0=param, 1=copy, 2=move

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
    Obj a(42);
    if (a.value != 42) return 1;
    if (a.how != 0) return 2;

    // Copy init from lvalue — should call copy ctor
    Obj b = a;
    if (b.value != 42) return 3;
    if (b.how != 1) return 4;
    if (a.value != 42) return 5;  // source unchanged

    // Move init from rvalue — should call move ctor
    Obj c = static_cast<Obj&&>(a);
    if (c.value != 42) return 6;
    if (c.how != 2) return 7;
    if (a.value != -1) return 8;  // source marked moved-from

    return 0;
}
