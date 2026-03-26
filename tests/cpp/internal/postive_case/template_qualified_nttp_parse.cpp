// Parse test: qualified typedef NTTPs should parse in template parameter lists.
namespace ns {
using size_t = unsigned long;
}

template <typename T, ns::size_t N>
struct Buffer {
  T data[N];
};

int main() {
  return 0;
}
