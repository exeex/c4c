// Runtime test: prefix operator++ member call on a struct.
struct Counter {
  int val;

  void operator++() {
    val = val + 1;
  }
};

int main() {
  Counter c;
  c.val = 5;
  ++c;
  if (c.val != 6) return 1;
  ++c;
  if (c.val != 7) return 2;
  return 0;
}
