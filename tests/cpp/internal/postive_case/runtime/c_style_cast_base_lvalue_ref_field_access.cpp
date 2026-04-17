// Regression: field access through a C-style cast to a base lvalue reference
// must preserve the original derived object storage for both reads and writes.

struct Base {
  int value;
};

struct Derived : Base {};

int main() {
  Derived d{};

  ((Base&)d).value = 7;

  return ((Base&)d).value == 7 ? 0 : 2;
}
