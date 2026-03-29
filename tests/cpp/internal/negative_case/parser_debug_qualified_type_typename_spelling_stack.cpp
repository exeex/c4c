// Parser-debug regression: keep the dependent-typename spelling helper visible
// when a later top-level parameter-list failure becomes the committed root
// cause for a nested qualified template argument.

namespace ns {
    template <class T>
    struct Holder {
        using type = T;
    };

    template <class T>
    struct Box {};
}

ns::Box<typename ns::Holder<ns::Box<int>>::type> value(
