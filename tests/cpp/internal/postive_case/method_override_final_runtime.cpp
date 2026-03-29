// Runtime regression: in-class virt-specifiers must be consumed by the member
// declaration suffix so the method body is retained instead of being dropped.

struct Base {
    virtual int value() { return 1; }
};

struct Derived final : Base {
    int value() override final { return 7; }
};

int main() {
    Derived d;
    return d.value() - 7;
}
