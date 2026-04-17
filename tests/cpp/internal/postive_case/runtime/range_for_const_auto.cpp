// Range-for Phase 1: const auto type deduction.
// Validates: for (const auto x : container) with const begin/end
struct ConstIter {
  const int* p;

  int operator*() const {
    return *p;
  }

  ConstIter operator++() {
    p = p + 1;
    ConstIter next;
    next.p = p;
    return next;
  }

  bool operator!=(ConstIter other) const {
    return p != other.p;
  }
};

struct Container {
  int buf[3];
  int len;

  ConstIter begin() const {
    ConstIter it;
    it.p = buf;
    return it;
  }

  ConstIter end() const {
    ConstIter it;
    it.p = buf + len;
    return it;
  }
};

int main() {
  Container c;
  c.buf[0] = 5;
  c.buf[1] = 15;
  c.buf[2] = 25;
  c.len = 3;

  // const auto deduced as const int
  int sum = 0;
  for (const auto x : c) {
    sum = sum + x;
  }
  if (sum != 45) return 1;

  return 0;
}
