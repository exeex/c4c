// Phase 1: Iterator class basics — declaration, field access, method calls.
struct Iter {
  int* ptr;
  int index;

  int value() {
    return ptr[index];
  }

  bool at_end(int len) {
    return index >= len;
  }
};

int main() {
  int arr[3];
  arr[0] = 10;
  arr[1] = 20;
  arr[2] = 30;

  Iter it;
  it.ptr = arr;
  it.index = 0;

  if (it.value() != 10) return 1;
  if (it.at_end(3)) return 2;

  it.index = 2;
  if (it.value() != 30) return 3;
  if (it.at_end(3)) return 4;

  it.index = 3;
  if (!it.at_end(3)) return 5;

  return 0;
}
