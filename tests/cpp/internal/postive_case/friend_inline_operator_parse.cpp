// Parse-only regression: friend operator definitions inside a class body
// must not terminate the enclosing class parse at the function body's `}`.

template<typename T>
struct Box {
  T value;

  friend bool operator==(const Box& a, const Box& b) noexcept {
    return a.value == b.value;
  }

  friend bool operator!=(const Box& a, const Box& b) noexcept {
    return !(a == b);
  }

  static int marker() { return 7; }
};

int main() {
  Box<int> a{1};
  Box<int> b{1};
  return (a == b) ? 0 : Box<int>::marker();
}
