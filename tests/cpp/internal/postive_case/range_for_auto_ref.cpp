// Range-for Phase 2: auto& and const auto& reference loop variables.
// Validates: for (auto& x : container), for (const auto& x : container)
struct Iter {
  int* p;

  int& operator*() {
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

  // Test 1: for (auto& x : c) — modify elements through reference
  for (auto& x : c) {
    x = x * 2;
  }
  // Check that modifications took effect
  if (c.buf[0] != 20) return 1;
  if (c.buf[1] != 40) return 2;
  if (c.buf[2] != 60) return 3;
  if (c.buf[3] != 80) return 4;

  // Test 2: for (const auto& x : c) — read elements through const reference
  int sum = 0;
  for (const auto& x : c) {
    sum = sum + x;
  }
  if (sum != 200) return 5;

  // Test 3: for (auto& x : c) — take address of reference
  int* first_ptr = 0;
  for (auto& x : c) {
    first_ptr = &x;
    break;
  }
  if (first_ptr != c.buf) return 6;

  return 0;
}
