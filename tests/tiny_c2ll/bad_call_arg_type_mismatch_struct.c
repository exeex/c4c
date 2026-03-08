// Expected: compile fail (incompatible struct argument type)
struct A { int x; };
struct B { int x; };

int takes_a(struct A a) {
  return a.x;
}

int main(void) {
  struct B b;
  b.x = 7;
  return takes_a(b);
}
