template<int M = true>
struct Buffer {
    int data[M];
};

Buffer<> global_buffer;

int main() {
    return global_buffer.data[0];
}
