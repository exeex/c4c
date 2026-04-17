namespace std {
template<typename T>
inline constexpr bool is_array_v = false;
}

template<typename T>
concept member_begin = true;

template<typename T>
concept adl_begin = true;

template<typename T>
requires std::is_array_v<T> || member_begin<T&> || adl_begin<T&>
auto begin_like(T& t) {
    return begin(t);
}

int main() {
    return 0;
}
