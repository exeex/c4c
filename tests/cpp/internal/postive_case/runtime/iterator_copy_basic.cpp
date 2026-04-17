// Phase 1: Iterator copy — init from another iterator, assignment.
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

  bool operator!=(Iter other) {
    return p != other.p;
  }
};

int main() {
  int arr[3];
  arr[0] = 5;
  arr[1] = 15;
  arr[2] = 25;

  Iter orig;
  orig.p = arr;

  // Copy initialization
  Iter copy = orig;
  if (*copy != 5) return 1;

  // Advancing copy should not affect orig
  ++copy;
  if (*copy != 15) return 2;
  if (*orig != 5) return 3;  // orig unchanged

  // Copy assignment
  Iter other;
  other.p = arr + 2;
  if (*other != 25) return 4;
  other = orig;  // assign from orig
  if (*other != 5) return 5;

  return 0;
}
