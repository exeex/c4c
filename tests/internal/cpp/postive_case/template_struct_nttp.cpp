// Template struct with non-type template parameters (NTTP)
template<typename T, int N>
struct Array {
    T data[N];
};

template<int SIZE>
struct IntBuf {
    int buf[SIZE];
};

int main() {
    // Basic NTTP struct: Array<int, 4>
    Array<int, 4> a;
    a.data[0] = 10;
    a.data[1] = 20;
    a.data[2] = 30;
    a.data[3] = 40;
    if (a.data[0] + a.data[3] != 50) return 1;

    // Different instantiation: Array<long, 2>
    Array<long, 2> b;
    b.data[0] = 100L;
    b.data[1] = -58L;
    if (b.data[0] + b.data[1] != 42L) return 2;

    // NTTP-only struct: IntBuf<3>
    IntBuf<3> ib;
    ib.buf[0] = 1;
    ib.buf[1] = 2;
    ib.buf[2] = 39;
    if (ib.buf[0] + ib.buf[1] + ib.buf[2] != 42) return 3;

    return 0;
}
