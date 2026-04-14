// Parse regression: C++ alignas must accept a symbolic constant expression.
constexpr unsigned kBoxAlign = 16;

struct alignas(kBoxAlign) Box {
  int value;

  Box() : value(0) {}
  explicit Box(int v) : value(v) {}
};

int main() {
  Box box(7);
  return box.value;
}
