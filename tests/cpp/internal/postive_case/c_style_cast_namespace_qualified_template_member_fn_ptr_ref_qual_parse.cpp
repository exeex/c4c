// Parse-only: namespace-qualified template-id owners should remain
// declarations when trailing member-function ref-qualifiers appear inside a
// parenthesized C-style cast.
// RUN: %c4cll --parse-only %s

namespace ns {
template <class T>
struct Box {
    int lref(T) &;
    int rref(T) &&;
};
}

int main() {
    void* raw = 0;
    int (ns::Box<int>::*lref_fp)(int) & = (int (ns::Box<int>::*)(int) &)raw;
    int (ns::Box<int>::*rref_fp)(int) && = (int (ns::Box<int>::*)(int) &&)raw;
    return lref_fp == 0 && rref_fp == 0 ? 0 : 1;
}
