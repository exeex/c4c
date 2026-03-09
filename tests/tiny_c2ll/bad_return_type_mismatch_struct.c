// Expected: compile fail (incompatible return type)
struct A { int x; };
struct B { int x; };

struct A f(void) {
  struct B b;
  b.x = 1;
  return b;
}

int main(void) {
  struct A a = f();
  return a.x;
}
