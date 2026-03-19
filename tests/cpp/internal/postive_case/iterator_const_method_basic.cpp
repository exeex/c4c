// Phase 1: Const method on iterator type.
struct Iter {
  int* p;
  int len;

  int get() const {
    return *p;
  }

  bool valid() const {
    return len > 0;
  }
};

int main() {
  int arr[2];
  arr[0] = 77;
  arr[1] = 88;

  Iter it;
  it.p = arr;
  it.len = 2;

  if (it.get() != 77) return 1;
  if (!it.valid()) return 2;

  it.p = arr + 1;
  it.len = 0;
  if (it.get() != 88) return 3;
  if (it.valid()) return 4;

  return 0;
}
