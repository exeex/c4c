// Constructor initializer list: member constructor calls
// Tests: struct member initialized via its constructor in the init list,
//        multi-arg member constructor, default-constructed members,
//        mixed scalar + struct member initialization.

struct Inner {
    int val;

    Inner(int v) : val(v) {}
};

struct Pair {
    int x;
    int y;

    Pair(int a, int b) : x(a), y(b) {}

    int sum() const {
        return x + y;
    }
};

// Single struct member initialized by constructor
struct Wrapper {
    Inner item;

    Wrapper(int v) : item(v) {}
};

// Mixed scalar + struct member
struct Container {
    int tag;
    Inner data;

    Container(int t, int d) : tag(t), data(d) {}
};

// Struct member with multi-arg constructor
struct Box {
    Pair coords;
    int label;

    Box(int x, int y, int l) : coords(x, y), label(l) {}
};

// Multiple struct members
struct TwoInner {
    Inner first;
    Inner second;

    TwoInner(int a, int b) : first(a), second(b) {}
};

// Struct member + body code
struct Hybrid {
    Inner item;
    int computed;

    Hybrid(int v) : item(v) {
        computed = v * 10;
    }
};

int main() {
    // Single struct member via constructor
    Wrapper w(42);
    if (w.item.val != 42) return 1;

    // Mixed scalar + struct member
    Container c(7, 99);
    if (c.tag != 7) return 2;
    if (c.data.val != 99) return 3;

    // Multi-arg member constructor
    Box b(3, 4, 5);
    if (b.coords.x != 3) return 4;
    if (b.coords.y != 4) return 5;
    if (b.coords.sum() != 7) return 6;
    if (b.label != 5) return 7;

    // Multiple struct members
    TwoInner t(10, 20);
    if (t.first.val != 10) return 8;
    if (t.second.val != 20) return 9;

    // Struct member + body code
    Hybrid h(6);
    if (h.item.val != 6) return 10;
    if (h.computed != 60) return 11;

    return 0;
}
