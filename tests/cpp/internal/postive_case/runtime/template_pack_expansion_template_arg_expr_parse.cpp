// Parse-only coverage for a template-id pack expansion used as a nested
// template argument expression.

template <typename... Ts>
struct all_true {
    static constexpr bool value = true;
};

template <typename T>
struct trait {
    static constexpr bool value = true;
};

template <typename... Ts>
struct probe {
    static constexpr bool value = all_true<trait<Ts>..., all_true<Ts...>>::value;
};

int main() {
    (void)probe<int, char>::value;
    return 0;
}
