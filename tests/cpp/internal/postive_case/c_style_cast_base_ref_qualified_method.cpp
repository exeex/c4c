// Regression: method dispatch through a C-style cast to a base reference must
// preserve both inherited owner lookup and ref-qualified value category.

struct Base {
  int select() & { return 1; }
  int select() && { return 2; }
};

struct Derived : Base {};

int main() {
  Derived d;

  if (((Base&)d).select() != 1) return 1;
  if (((Base&&)d).select() != 2) return 2;

  return 0;
}
