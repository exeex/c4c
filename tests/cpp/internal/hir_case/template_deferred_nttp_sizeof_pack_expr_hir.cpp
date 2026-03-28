template<typename... Ts, int M = sizeof...(Ts)>
struct Buffer {
    int data[M];
};

Buffer<int, char, long> global_buffer;

int main() {
    return global_buffer.data[2];
}
