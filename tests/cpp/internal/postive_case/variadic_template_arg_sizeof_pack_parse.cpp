// Runtime coverage for sizeof...(pack) as a non-type template argument.
// This verifies both parsing and value propagation through a template-id.

template<int N>
struct X {
    static constexpr int value = N;
};

template<typename... Ts>
int pack_size_value() {
    return X<sizeof...(Ts)>::value;
}

int main() {
    if (pack_size_value<>() != 0) return 1;
    if (pack_size_value<int>() != 1) return 2;
    if (pack_size_value<int, char, long>() != 3) return 3;
    return 0;
}
