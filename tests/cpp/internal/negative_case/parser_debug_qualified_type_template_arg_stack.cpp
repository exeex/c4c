// Parser-debug regression: keep the nested qualified-type template-argument
// path visible when a later top-level parameter-list failure becomes the
// committed root cause.

namespace ns {
    template <class T>
    struct Box {};

    struct S {};
}

ns::Box<ns::S> value(
