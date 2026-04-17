// Parse test: typedef-like NTTP heads should parse the same whether the type
// name is plain, qualified, or globally qualified.

using size_t = unsigned long;

namespace ns {
using size_t = unsigned long;
}

template <typename T, size_t N>
struct local_buffer {
    T data[N];
};

template <typename T, ns::size_t N>
struct qualified_buffer {
    T data[N];
};

template <typename T, ::size_t N>
struct global_buffer {
    T data[N];
};

int main() {
    local_buffer<int, 1> a{};
    qualified_buffer<int, 2> b{};
    global_buffer<int, 3> c{};
    return a.data[0] + b.data[0] + c.data[0];
}
