// Tests copy initialization: T b = a; calls copy constructor instead of memcpy.
struct Obj {
    int value;
    int how;  // 0=param ctor, 1=copy ctor

    Obj(int v) {
        value = v;
        how = 0;
    }

    // Copy constructor — adds 100 to distinguish from plain memcpy
    Obj(const Obj& other) {
        value = other.value + 100;
        how = 1;
    }
};

int main() {
    Obj a(42);
    if (a.value != 42) return 1;
    if (a.how != 0) return 2;

    // Copy initialization via = should call copy constructor
    Obj b = a;
    if (b.value != 142) return 3;  // 42 + 100 from copy ctor
    if (b.how != 1) return 4;

    // Source unchanged
    if (a.value != 42) return 5;

    // Copy from modified object
    a.value = 10;
    Obj c = a;
    if (c.value != 110) return 6;  // 10 + 100
    if (c.how != 1) return 7;

    return 0;
}
