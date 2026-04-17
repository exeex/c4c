// Parse test: forwarded NTTP name in type-context template instantiation.
template <int N>
struct Box {};

template <int N>
struct Holder {
  Box<N>* ptr;
};

int main() {
  return 0;
}
