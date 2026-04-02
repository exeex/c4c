// Parse-only regression: statement parsing should treat a typedef-owner
// qualified operator call as an expression, not a local declaration.
// RUN: %c4cll --parse-only %s

struct BaseImpl {
    BaseImpl& operator=(const BaseImpl&) {
        return *this;
    }
};

struct Derived : BaseImpl {
    using Base = BaseImpl;

    void assign_from(const Base& other) {
        Base::operator=(other);
    }
};

int main() {
    Derived value;
    value.assign_from(value);
    return 0;
}
