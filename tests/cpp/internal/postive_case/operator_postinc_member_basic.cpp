// Runtime test: postfix operator++(int) member call on a struct.
struct Counter {
  int val;

  Counter operator++(int) {
    Counter old;
    old.val = val;
    val = val + 1;
    return old;
  }
};

int main() {
  Counter c;
  c.val = 5;
  Counter prev = c++;
  if (prev.val != 5) return 1;
  if (c.val != 6) return 2;
  prev = c++;
  if (prev.val != 6) return 3;
  if (c.val != 7) return 4;
  return 0;
}
