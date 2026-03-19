// Phase 1 negative: rvalue reference cannot bind to lvalue.
int main() {
  int x = 42;
  int&& r = x;
  return r;
}
