// Parse-only coverage for variadic templates nested inside class bodies.

template <typename T>
struct Wrapper {
    T value;

    template <typename... Args>
    struct Rebind {};

    template <bool... Flags>
    struct Mask {};
};

template <typename... Ts>
struct Outer {
    template <typename First, typename... Rest>
    struct Inner {
        First first;
    };
};

int main() {
    return 0;
}
