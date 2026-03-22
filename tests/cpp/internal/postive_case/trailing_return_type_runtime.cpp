// Runtime test: C++ trailing return types should set the function return type.
template<typename F>
auto wrap(F f) -> decltype(f()) {
  return f();
}

struct Box {
  int value;

  auto get() -> int {
    return value;
  }
};

int one() {
  return 1;
}

int main() {
  Box b;
  b.value = 41;
  return wrap(one) + b.get() - 42;
}
