// Negative test: unsupported operator overload token should error.
struct Foo {
  int operator%(int x) {
    return x;
  }
};

int main() {
  return 0;
}
