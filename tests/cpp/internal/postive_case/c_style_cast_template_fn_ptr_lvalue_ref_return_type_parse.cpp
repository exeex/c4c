// Parse-only: template-id lvalue-reference return types should remain
// declarations when they feed a parenthesized function-pointer declarator
// inside a C-style cast.
// RUN: %c4cll --parse-only %s

template <class T>
struct Box {
    T value;
};

int main() {
    void* raw = 0;
    Box<int>& (*fp)(int) = (Box<int>& (*)(int))raw;
    return fp == 0 ? 0 : 1;
}
