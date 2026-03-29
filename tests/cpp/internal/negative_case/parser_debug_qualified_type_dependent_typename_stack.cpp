// Parser-debug regression: keep the dependent-typename helper visible when a
// later top-level parameter-list failure becomes the committed root cause.

namespace ns {
    template <class T>
    struct Holder {
        using type = T;
    };

    template <class T>
    struct Box {};
}

ns::Box<typename ns::Holder<int>::type> value(
