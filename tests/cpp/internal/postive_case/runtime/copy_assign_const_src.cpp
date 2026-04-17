// Copy assignment from const source object.
// Tests: operator=(const T&) correctly reads from a const source.
struct Data {
    int x;
    int y;

    Data(int a, int b) {
        x = a;
        y = b;
    }

    Data& operator=(const Data& other) {
        x = other.x;
        y = other.y;
        return *this;
    }

    int sum() const { return x + y; }
};

int main() {
    const Data src(10, 20);
    Data dst(0, 0);

    if (src.sum() != 30) return 1;
    if (dst.sum() != 0) return 2;

    // Copy from const source
    dst = src;
    if (dst.sum() != 30) return 3;
    if (src.sum() != 30) return 4;  // src unchanged

    // Copy from another non-const
    Data other(5, 15);
    dst = other;
    if (dst.sum() != 20) return 5;

    return 0;
}
