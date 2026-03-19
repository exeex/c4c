// Delegating constructor support
// A constructor can delegate to another constructor of the same class
// using the init-list syntax: Ctor(args) : ClassName(other_args) { body }

extern "C" int printf(const char*, ...);

struct Point {
    int x;
    int y;

    Point(int a, int b) : x(a), y(b) {}

    // Delegating: single-arg delegates to two-arg
    Point(int a) : Point(a, 0) {}

    // Delegating: zero-arg delegates to single-arg
    Point() : Point(42) {}
};

struct Counter {
    int val;
    int step;

    Counter(int v, int s) : val(v), step(s) {}

    // Delegating with expression in args
    Counter(int v) : Counter(v, 1) {}

    // Chain: zero-arg -> single-arg -> two-arg
    Counter() : Counter(0) {}
};

// Delegating ctor with body that runs AFTER the target ctor
struct Logger {
    int value;
    int logged;

    Logger(int v) : value(v), logged(0) {}

    Logger() : Logger(99) {
        // Body runs after delegation completes
        logged = 1;
    }
};

int main() {
    // Two-arg constructor (non-delegating)
    Point p1(10, 20);
    if (p1.x != 10 || p1.y != 20) return 1;

    // Single-arg delegating to two-arg
    Point p2(5);
    if (p2.x != 5 || p2.y != 0) return 2;

    // Zero-arg delegating through chain
    Point p3;
    if (p3.x != 42 || p3.y != 0) return 3;

    // Counter: two-arg
    Counter c1(10, 2);
    if (c1.val != 10 || c1.step != 2) return 4;

    // Counter: single-arg delegating
    Counter c2(7);
    if (c2.val != 7 || c2.step != 1) return 5;

    // Counter: zero-arg chain delegation
    Counter c3;
    if (c3.val != 0 || c3.step != 1) return 6;

    // Logger: delegating ctor with body
    Logger l1(50);
    if (l1.value != 50 || l1.logged != 0) return 7;

    Logger l2;
    if (l2.value != 99) return 8;
    if (l2.logged != 1) return 9;

    printf("OK\n");
    return 0;
}
