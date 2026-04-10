// HIR regression: non-type template argument materialization should preserve
// static-member const evaluation when the value expression feeds another
// template instantiation directly.

template <int N>
struct Count {
  static constexpr int value = N;
};

template <int V>
struct Buffer {
  int data[V];
};

Buffer<Count<3>::value + 1> global_buffer;

int main() {
  return global_buffer.data[3];
}
