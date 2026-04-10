// Frontend regression: an out-of-class constructor initializer should accept
// a qualified owner-member typedef functional cast.

struct Base {
  typedef int allocator_type;
};

struct Derived : Base {
  typedef Base base_type;
  int value;
  Derived();
};

Derived::Derived() : value(base_type::allocator_type(7)) {}

int main() {
  Derived value;
  return value.value == 7 ? 0 : 1;
}
