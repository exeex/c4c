// HIR regression: template struct body instantiation should keep NTTP array
// extents, inherited member access, and instantiated method lowering concrete
// after the helper family moves behind its own translation unit.

template<typename T, int N>
struct Base {
  T value;
};

template<typename T, int N>
struct Buffer : Base<T, N> {
  T data[N];
  T tail;

  T sum() const {
    return this->value + data[1] + tail;
  }
};

int main() {
  Buffer<int, 3> b;
  b.value = 10;
  b.data[1] = 20;
  b.tail = 12;
  return b.sum() - 42;
}
