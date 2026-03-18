// Parse test: operator* declaration in a struct (unary dereference).
struct Iter {
  int* ptr;

  int operator*() {
    return *ptr;
  }
};

int main() {
  return 0;
}
