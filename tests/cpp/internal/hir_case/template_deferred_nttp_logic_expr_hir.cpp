template<int N, int M = N == 0 || N == 3>
struct Buffer {
    int data[M];
};

Buffer<3> global_buffer;

int main() {
    return global_buffer.data[0];
}
