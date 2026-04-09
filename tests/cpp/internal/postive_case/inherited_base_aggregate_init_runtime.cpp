// Runtime regression: aggregate initialization of a derived object must
// populate its non-virtual base subobject storage.

struct Base {
    int value;
};

struct Derived : Base {};

static int read_base(const Base& base) {
    return base.value;
}

int main() {
    Derived derived{{1}};
    return read_base(derived) == 1 ? 0 : 1;
}
