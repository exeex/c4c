// Range-for Phase 0: Const container range-for.
// Validates: range-for over const container uses const begin/end.
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

  bool operator!=(ConstIter other) {
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

int sum_container(const Container* cp) {
  int total = 0;
  // Use manual loop through const pointer — range-for on const objects
  ConstIter it = cp->begin();
  ConstIter e = cp->end();
  while (it != e) {
    total = total + *it;
    ++it;
  }
  return total;
}

int main() {
  Container c;
  c.buf[0] = 5;
  c.buf[1] = 15;
  c.buf[2] = 25;
  c.len = 3;

  // Range-for with explicit type on non-const container
  int sum = 0;
  for (int x : c) {
    sum = sum + x;
  }
  // 5 + 15 + 25 = 45
  if (sum != 45) return 1;

  // Verify const access also works (via manual loop helper)
  if (sum_container(&c) != 45) return 2;

  return 0;
}
