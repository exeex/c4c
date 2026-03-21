// Test: default parameter values are parsed without error
// Mode: parse-only

int add(int a, int b = 10);
int multi(int a, int b = 2, int c = 3);
void takes_ptr(const char* s = nullptr);
void complex_default(int x = (1 + 2) * 3);

struct Foo {
    Foo(int v = 42);
    int get(int offset = 0) const;
    void set(int a, int b = -1);
};

// Out-of-class definition (no default repeated)
int add(int a, int b) { return a + b; }

int main() {
    add(5);
    add(5, 20);
    return 0;
}
