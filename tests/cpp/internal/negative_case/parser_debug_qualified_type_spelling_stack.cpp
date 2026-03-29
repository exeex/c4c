// Parser-debug regression: keep the nested template-argument speculative
// dispatch visible when a later top-level parameter-list failure becomes the
// committed root cause for a qualified template argument without `typename`.

namespace ns {
    template <class T>
    struct Holder {
        using type = T;
    };

    template <class T>
    struct Box {};
}

ns::Holder<ns::Box<int>>::type value(
