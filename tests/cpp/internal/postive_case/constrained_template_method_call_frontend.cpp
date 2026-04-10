// Reduced frontend regression from EASTL/libstdc++ tuple follow-up: concept
// shorthand template parameters must still register the template primary so
// later `Box<int>` method calls instantiate and lower correctly.

template<typename T>
concept C = true;

template<C It>
struct Box {
  Box& advance(int) { return *this; }
};

int main() {
  Box<int> b{};
  b.advance(1);
  return 0;
}
