// Default constructor: T var; calls zero-arg constructor when defined.
// Tests: default constructor declaration, implicit invocation via T var;,
//        overload with parameterized constructor.
struct Counter {
    int value;
    int initialized;

    Counter() {
        value = 0;
        initialized = 1;
    }

    Counter(int v) {
        value = v;
        initialized = 1;
    }

    int get() const {
        return value;
    }
};

struct Pair {
    int x;
    int y;

    Pair() {
        x = 10;
        y = 20;
    }
};

int main() {
    // T var; should call default constructor
    Counter c;
    if (c.initialized != 1) return 1;
    if (c.get() != 0) return 2;

    // T var(args) still works for parameterized constructor
    Counter d(42);
    if (d.initialized != 1) return 3;
    if (d.get() != 42) return 4;

    // Another struct with default constructor
    Pair p;
    if (p.x != 10) return 5;
    if (p.y != 20) return 6;

    return 0;
}
