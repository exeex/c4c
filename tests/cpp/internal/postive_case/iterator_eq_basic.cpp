// Phase 2: Iterator operator== for equality comparison.
struct Iter {
  int* p;

  bool operator==(Iter other) {
    return p == other.p;
  }

  bool operator!=(Iter other) {
    return p != other.p;
  }
};

int main() {
  int arr[3];
  arr[0] = 0;
  arr[1] = 0;
  arr[2] = 0;

  Iter a;
  a.p = arr;
  Iter b;
  b.p = arr;

  // Same position — equal
  if (!(a == b)) return 1;

  // Different positions — not equal
  Iter c;
  c.p = arr + 1;
  if (a == c) return 2;

  // Advance to same position
  a.p = arr + 1;
  if (!(a == c)) return 3;

  return 0;
}
