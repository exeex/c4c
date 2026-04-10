// Parse-only: namespace-qualified dependent owner chains with a non-final
// template-id segment should remain type-ids when they appear inside a
// parenthesized const member-function-pointer C-style cast.
// RUN: %c4cll --parse-only %s

namespace ns {
template <class T>
struct Holder {
    template <class U>
    struct Rebind {
        struct Inner {
            int method(U) const;
        };
    };
};
}

template <class T>
void probe() {
    auto fp = (int (ns::Holder<T>::template Rebind<T>::Inner::*)(T) const)0;
    (void)fp;
}

int main() {
    return 0;
}
