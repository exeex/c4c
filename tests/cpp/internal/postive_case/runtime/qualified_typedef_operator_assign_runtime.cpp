// Runtime regression: a typedef-owner qualified operator= call in statement
// position must stay an expression so the base assignment actually executes.

int g_assign_calls = 0;

struct BaseImpl {
    BaseImpl& operator=(const BaseImpl&) {
        g_assign_calls = g_assign_calls + 1;
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
    Derived lhs;
    Derived rhs;
    lhs.assign_from(rhs);
    return g_assign_calls == 1 ? 0 : 1;
}
