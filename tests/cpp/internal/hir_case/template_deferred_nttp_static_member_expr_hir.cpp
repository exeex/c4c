template<int N>
struct Count {
    static constexpr int value = N;
};

template<int N, int M = Count<N>::value + 2>
struct Buffer {
    int data[M];
};

Buffer<3> global_buffer;

int main() {
    return global_buffer.data[4];
}
