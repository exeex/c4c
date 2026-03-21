// Parse-only coverage for mixed variadic template parameter forms.

typedef unsigned long size_t;

template <typename Head, typename... Tail>
struct TypeList {
    Head head;
};

template <class... Ts>
struct TupleLike {};

template <typename T, T... Values>
struct ValuePack {};

template <size_t... Ns>
struct IndexSeq {};

template <bool... Bits>
struct BoolFlags {};

template <typename... Ts>
TupleLike<Ts...> make_tuple_like();

int main() {
    return 0;
}
