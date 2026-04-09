// Parse-only: dependent template-id owner chains should remain declarations
// when they appear inside a C-style cast targeting a data-member pointer.
// RUN: %c4cll --parse-only %s

namespace ns {
template <class T>
struct Box {
    T value;
};
}

template <class T>
struct Holder {
    using Type = ns::Box<T>;
};

template <class T>
int probe() {
    int Holder<T>::Type::*member = (int Holder<T>::Type::*)0;
    return member == 0 ? 0 : 1;
}

int main() {
    return probe<int>();
}
