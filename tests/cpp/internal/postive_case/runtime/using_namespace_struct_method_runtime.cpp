// Runtime test: using-directive should expose namespace-owned struct types and methods.
namespace lib {
struct Box {
  int value;
  int get() const { return value; }
};
}

using namespace lib;

int main() {
  Box b;
  b.value = 27;
  return b.get() - 27;
}
