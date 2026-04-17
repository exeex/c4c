// Runtime test: unqualified static member function calls inside out-of-class method bodies.
// Inside B::foo(), calling StaticAdd(a, b) should resolve to B::StaticAdd(a, b).
struct B {
  int x;

  B() : x(0) {}
  B(int v) : x(v) {}

  static int StaticAdd(int a, int b);
  int compute(int y);
};

int B::StaticAdd(int a, int b) {
  return a + b;
}

int B::compute(int y) {
  // Unqualified call to static member function
  int sum = StaticAdd(x, y);
  return sum;
}

int main() {
  B obj(10);
  int r = obj.compute(32);
  if (r != 42) return 1;
  return 0;
}
