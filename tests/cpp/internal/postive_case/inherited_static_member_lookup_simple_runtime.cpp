struct Base {
    static constexpr int value = 7;
};

struct Derived : Base {};

int main() {
    return Derived::value == 7 ? 0 : 1;
}
