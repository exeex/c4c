// Phase 0: Iterator contract smoke test.
// Validates the minimum iterator shape: operator*, prefix operator++, operator!=.
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

  bool operator==(Iter other) {
    return p == other.p;
  }
};

int main() {
  int arr[3];
  arr[0] = 100;
  arr[1] = 200;
  arr[2] = 300;

  Iter it;
  it.p = arr;

  // operator* works
  if (*it != 100) return 1;

  // prefix operator++ works
  ++it;
  if (*it != 200) return 2;

  ++it;
  if (*it != 300) return 3;

  // operator!= works
  Iter a;
  a.p = arr;
  Iter b;
  b.p = arr;
  if (a != b) return 4;  // same pointer, should be equal

  Iter c;
  c.p = arr + 1;
  if (!(a != c)) return 5;  // different pointer, should be not-equal

  // operator== works
  if (!(a == b)) return 6;
  if (a == c) return 7;

  return 0;
}
