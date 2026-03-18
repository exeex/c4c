// Parse test: operator-> declaration in a struct.
struct Wrapper {
  int value;
  int* operator->() {
    return &value;
  }
};

int main() {
  return 0;
}
