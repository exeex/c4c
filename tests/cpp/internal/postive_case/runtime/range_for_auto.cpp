// Range-for Phase 1: auto type deduction for loop variable.
// Validates: for (auto x : container), for (auto x : container) with struct elements
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

  // Test 1: for (auto x : c) — auto deduced as int
  int sum = 0;
  for (auto x : c) {
    sum = sum + x;
  }
  if (sum != 100) return 1;

  // Test 2: for (auto x : c) again — idempotent
  int count = 0;
  for (auto val : c) {
    count = count + 1;
  }
  if (count != 4) return 2;

  return 0;
}
