// Runtime regression: direct member access on a derived object must find
// inherited non-static data members from a non-virtual base.

struct Base {
    int value;
};

struct Derived : Base {};

int main() {
    Derived derived{{1}};
    return derived.value == 1 ? 0 : 1;
}
