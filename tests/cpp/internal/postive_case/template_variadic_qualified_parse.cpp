// Parse-only coverage for qualified variadic template references.

template <typename... Ts>
struct PackHolder {};

template <typename T>
struct Box {
    template <typename... Args>
    struct Nested {};
};

template <typename... Ts>
struct UseQualified {
    Box<int>::Nested<Ts...>* nested_ptr;
    PackHolder<Ts...>* pack_ptr;
};

int main() {
    return 0;
}
