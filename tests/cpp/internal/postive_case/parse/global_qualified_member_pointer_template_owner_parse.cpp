// Parse-only: the shared `::*` owner path should accept global-qualified
// templated owners in both plain and parenthesized member-pointer declarators.
// RUN: %c4cll --parse-only %s

namespace ns {
template <typename T>
struct Box {
    int value;
    int get() const { return value; }
};
}

template <typename T>
void accept_member(int ::ns::Box<T>::*member) {
    (void)member;
}

template <typename T>
void accept_method(int (::ns::Box<T>::*pm)() const) {
    (void)pm;
}

int main() {
    return 0;
}
