// Tests = default syntax for special member functions.

int g_dtor_count = 0;

struct Simple {
    int x;
    int y;

    Simple() = default;
    Simple(int a, int b) : x(a), y(b) {}
    Simple(const Simple&) = default;
    Simple& operator=(const Simple&) = default;
    ~Simple() = default;
};

struct WithDtor {
    int val;
    WithDtor(int v) : val(v) {}
    ~WithDtor() { g_dtor_count = g_dtor_count + 1; }
};

struct Outer {
    WithDtor inner;
    int extra;

    Outer(int v, int e) : inner(v), extra(e) {}
    // Default destructor should still call inner's dtor
    ~Outer() = default;
};

struct Movable {
    int data;
    int moved;  // 0=normal, 1=moved-from

    Movable(int d) : data(d), moved(0) {}
    Movable(const Movable&) = default;
    Movable(Movable&&) = default;
    Movable& operator=(const Movable&) = default;
    Movable& operator=(Movable&&) = default;
};

int main() {
    // Test 1: default constructor (zero-init)
    Simple s1;
    // Note: = default ctor may not zero-init (C++ standard says indeterminate),
    // but our implementation does alloca + no init, which may be zero on first use.
    // We just test it doesn't crash.

    // Test 2: parameterized ctor still works
    Simple s2(10, 20);
    if (s2.x != 10) return 1;
    if (s2.y != 20) return 2;

    // Test 3: default copy constructor (memberwise copy)
    Simple s3 = s2;
    if (s3.x != 10) return 3;
    if (s3.y != 20) return 4;

    // Test 4: default copy assignment
    Simple s4(1, 2);
    s4 = s2;
    if (s4.x != 10) return 5;
    if (s4.y != 20) return 6;
    // Source unchanged
    if (s2.x != 10) return 7;

    // Test 5: default destructor with member dtor
    g_dtor_count = 0;
    {
        Outer o(42, 7);
        if (o.inner.val != 42) return 8;
        if (o.extra != 7) return 9;
    }
    // Outer's defaulted dtor should still call inner's WithDtor dtor
    if (g_dtor_count != 1) return 10;

    // Test 6: default move constructor (memberwise copy)
    Movable m1(99);
    Movable m2 = static_cast<Movable&&>(m1);
    if (m2.data != 99) return 11;
    if (m2.moved != 0) return 12;

    // Test 7: default copy constructor
    Movable m3(55);
    Movable m4 = m3;
    if (m4.data != 55) return 13;

    // Test 8: default copy assignment
    Movable m5(0);
    m5 = m3;
    if (m5.data != 55) return 14;

    // Test 9: default move assignment
    Movable m6(77);
    Movable m7(0);
    m7 = static_cast<Movable&&>(m6);
    if (m7.data != 77) return 15;

    return 0;
}
