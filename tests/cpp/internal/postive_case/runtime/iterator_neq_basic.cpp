// Phase 2: Iterator operator!= for loop termination.
struct Iter {
  int* p;

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

  // Same position — not not-equal
  if (a != b) return 1;

  // Different positions — not-equal
  Iter c;
  c.p = arr + 2;
  if (!(a != c)) return 2;

  // Use in loop counting
  Iter it;
  it.p = arr;
  Iter end;
  end.p = arr + 3;
  int count = 0;
  while (it != end) {
    count = count + 1;
    it.p = it.p + 1;
  }
  if (count != 3) return 3;

  return 0;
}
