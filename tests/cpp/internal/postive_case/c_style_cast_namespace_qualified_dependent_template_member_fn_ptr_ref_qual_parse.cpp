// Parse-only: namespace-qualified dependent owner chains with a non-final
// template-id segment should remain type-ids when trailing member-function
// ref-qualifiers appear inside a parenthesized C-style cast.
// RUN: %c4cll --parse-only %s

namespace ns {
template <class T>
struct Holder {
    template <class U>
    struct Rebind {
        struct Inner {
            int lref(U) &;
            int rref(U) &&;
        };
    };
};
}

template <class T>
void probe() {
    auto lref_fp = (int (ns::Holder<T>::template Rebind<T>::Inner::*)(T) &)0;
    auto rref_fp = (int (ns::Holder<T>::template Rebind<T>::Inner::*)(T) &&)0;
    (void)lref_fp;
    (void)rref_fp;
}

int main() {
    return 0;
}
