// Parse test: dependent enum initializer should not be rejected in parser.
template <typename T>
struct TraitsAdapter {
  enum { value = bool(T::value) };
};

int main() {
  return 0;
}
