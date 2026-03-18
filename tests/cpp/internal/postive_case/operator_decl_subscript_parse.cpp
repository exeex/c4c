// Parse test: operator[] declaration in a struct.
struct Vec {
  int data[4];
  int size;

  int& operator[](int idx) {
    return data[idx];
  }
};

int main() {
  return 0;
}
