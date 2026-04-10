// Regression: field access through a C-style cast to a base rvalue reference
// must preserve xvalue category and original base-subobject storage.

struct Base {
  int value;
};

struct Derived : Base {};

int pick(int&) { return 1; }
int pick(int&&) { return 2; }

int main() {
  Derived d{};
  ((Base&)d).value = 1;

  if (((Base&&)d).value != 1) return 1;
  if (pick(((Base&&)d).value) != 2) return 2;

  int&& ref = ((Base&&)d).value;
  ref = 7;

  return ((Base&)d).value == 7 ? 0 : 3;
}
