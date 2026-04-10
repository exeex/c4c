// Parse-only: template-id parameter types should remain declarations when they
// appear inside a parenthesized function-pointer declarator in a C-style cast.
// RUN: %c4cll --parse-only %s

template <class T>
struct Box {
    T value;
};

int main() {
    void* raw = 0;
    int (*fp)(Box<int>) = (int (*)(Box<int>))raw;
    return fp == 0 ? 0 : 1;
}
