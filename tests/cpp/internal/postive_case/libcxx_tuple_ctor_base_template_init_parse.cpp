// Reduced from libc++ tuple: constructor initializer lists should accept
// template-id base names followed by pack-expanded calls.
// RUN: %c4cll --parse-only %s

template <int I, class T>
struct leaf {
  leaf(int, T&&);
};

template <int I, class... Ts>
struct tuple_impl : leaf<I, Ts>... {
  template <class Alloc, class Tuple, int = 0>
  tuple_impl(int, const Alloc& alloc, Tuple&& tuple_arg)
      : leaf<I, Ts>(0, static_cast<Ts&&>(tuple_arg))... {}
};

int main() {
  return 0;
}
