// Phase 4: const_iterator — separate iterator type for const access.
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

struct IntArray {
  int buf[8];
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

  int size() const {
    return len;
  }
};

int sum_const(const IntArray* a) {
  int s = 0;
  for (ConstIter it = a->begin(); it != a->end(); ++it) {
    s = s + *it;
  }
  return s;
}

int main() {
  IntArray a;
  a.buf[0] = 10;
  a.buf[1] = 20;
  a.buf[2] = 30;
  a.len = 3;

  // Non-const iterator — mutable access
  Iter it = a.begin();
  if (*it != 10) return 1;
  ++it;
  if (*it != 20) return 2;

  // Const iterator via const pointer
  if (sum_const(&a) != 60) return 3;

  // Const iterator directly
  const IntArray* cp = &a;
  ConstIter ci = cp->begin();
  if (*ci != 10) return 4;
  ++ci;
  if (*ci != 20) return 5;
  ++ci;
  if (*ci != 30) return 6;

  // Verify end
  ConstIter ce = cp->end();
  if (!(ci != ce)) return 7;
  ++ci;
  if (ci != ce) return 8;

  return 0;
}
