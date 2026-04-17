// Phase 0: Container with begin/end and manual iterator loop.
// Validates: for (Iter it = c.begin(); it != c.end(); ++it) { use *it }
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
  c.buf[0] = 1;
  c.buf[1] = 2;
  c.buf[2] = 3;
  c.buf[3] = 4;
  c.len = 4;

  // Manual iterator loop using for statement
  int sum = 0;
  for (Iter it = c.begin(); it != c.end(); ++it) {
    sum = sum + *it;
  }
  // 1 + 2 + 3 + 4 = 10
  if (sum != 10) return 1;

  // Reverse check: iterate and verify each element
  Iter it = c.begin();
  if (*it != 1) return 2;
  ++it;
  if (*it != 2) return 3;
  ++it;
  if (*it != 3) return 4;
  ++it;
  if (*it != 4) return 5;

  return 0;
}
