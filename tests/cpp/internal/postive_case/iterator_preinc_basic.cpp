// Phase 2: Iterator prefix operator++ stepping through elements.
struct Iter {
  int* p;

  int operator*() {
    return *p;
  }

  Iter operator++() {
    p = p + 1;
    Iter next;
    next.p = p;
    return next;
  }
};

int main() {
  int arr[4];
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  arr[3] = 4;

  Iter it;
  it.p = arr;

  if (*it != 1) return 1;
  ++it;
  if (*it != 2) return 2;
  ++it;
  if (*it != 3) return 3;
  ++it;
  if (*it != 4) return 4;

  return 0;
}
