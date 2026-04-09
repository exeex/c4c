// Parse-only: template-id owners should remain declarations when they appear
// inside a parenthesized const member-function-pointer declarator in a C-style
// cast.
// RUN: %c4cll --parse-only %s

template <class T>
struct Box {
    int method(T) const;
};

int main() {
    void* raw = 0;
    int (Box<int>::*fp)(int) const = (int (Box<int>::*)(int) const)raw;
    return fp == 0 ? 0 : 1;
}
