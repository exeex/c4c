// Range-for Phase 0: Basic range-based for loop.
// Validates: for (int x : container) { use x }
// Desugars to: begin/end + operator!=, operator++, operator*
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

struct Container {
  int buf[4];
  int len;

  Iter begin() {
    Iter it;
    it.p = buf;
    return it;
  }

  Iter end() {
    Iter it;
    it.p = buf + len;
    return it;
  }
};

int main() {
  Container c;
  c.buf[0] = 10;
  c.buf[1] = 20;
  c.buf[2] = 30;
  c.buf[3] = 40;
  c.len = 4;

  // Range-for with explicit type
  int sum = 0;
  for (int x : c) {
    sum = sum + x;
  }
  // 10 + 20 + 30 + 40 = 100
  if (sum != 100) return 1;

  // Range-for over same container again (idempotent)
  int count = 0;
  for (int val : c) {
    count = count + 1;
  }
  if (count != 4) return 2;

  return 0;
}
