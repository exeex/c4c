template<int N>
struct Count {
    static constexpr int value = N;
};

template<typename... Ts, int M = Count<3>::value + sizeof...(Ts) - 4>
struct Buffer {
    int data[M];
};

Buffer<int, char, long> global_buffer;

int main() {
    return global_buffer.data[1];
}
