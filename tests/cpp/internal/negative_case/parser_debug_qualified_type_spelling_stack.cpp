// Parser-debug regression: keep the fully spelled qualified-type helper path
// visible when a later top-level parameter-list failure becomes the committed
// root cause for a nested qualified template argument without `typename`.

namespace ns {
    template <class T>
    struct Holder {
        using type = T;
    };

    template <class T>
    struct Box {};
}

ns::Holder<ns::Box<int>>::type value(
