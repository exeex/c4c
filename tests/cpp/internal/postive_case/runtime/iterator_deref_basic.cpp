// Phase 2: Iterator operator* returning different types.
struct IntIter {
  int* p;

  int operator*() {
    return *p;
  }
};

struct PtrIter {
  int* p;

  int* operator*() {
    return p;
  }
};

int main() {
  int arr[2];
  arr[0] = 11;
  arr[1] = 22;

  // Value dereference
  IntIter vi;
  vi.p = arr;
  if (*vi != 11) return 1;

  // Pointer dereference — returns pointer, then dereference that
  PtrIter pi;
  pi.p = arr + 1;
  int* ptr = *pi;
  if (*ptr != 22) return 2;

  return 0;
}
