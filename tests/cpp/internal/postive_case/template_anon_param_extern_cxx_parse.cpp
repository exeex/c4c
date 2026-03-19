// Parse test: anonymous template params and extern "C++" linkage blocks.
extern "C++" {
template <typename>
struct TypeSink {};

template <bool>
struct BoolSink {};
}

int main() {
  return 0;
}
