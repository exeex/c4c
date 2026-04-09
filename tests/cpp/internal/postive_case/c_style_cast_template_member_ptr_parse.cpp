// Parse-only: template-id owners should remain declarations when they appear
// inside a parenthesized data-member-pointer declarator in a C-style cast.
// RUN: %c4cll --parse-only %s

template <class T>
struct Box {
    T value;
};

int main() {
    void* raw = 0;
    int Box<int>::*member = (int Box<int>::*)raw;
    return member == 0 ? 0 : 1;
}
