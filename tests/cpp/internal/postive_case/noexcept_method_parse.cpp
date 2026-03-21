// Test: noexcept on constructors, destructors, and methods in class bodies.
// Verifies the parser correctly skips noexcept/noexcept(expr)/throw() specifiers.

struct Foo {
    Foo() noexcept {}
    Foo(const Foo& other) noexcept {}
    Foo(Foo&& other) noexcept {}
    Foo& operator=(const Foo& other) noexcept { return *this; }
    Foo& operator=(Foo&& other) noexcept { return *this; }
    ~Foo() noexcept {}
    int get() const noexcept { return x; }
    void set(int v) noexcept(true) { x = v; }
    int x;
};

int main() {
    Foo a;
    a.set(42);
    return a.get() - 42;  // should return 0
}
