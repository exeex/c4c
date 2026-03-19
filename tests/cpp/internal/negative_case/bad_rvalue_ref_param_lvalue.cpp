// Phase 1 negative: rvalue reference parameter rejects lvalue argument.
void consume(int&& val) {}

int main() {
  int x = 42;
  consume(x);
  return 0;
}
