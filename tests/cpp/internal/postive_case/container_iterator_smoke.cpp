// Phase 3: Full container+iterator smoke test — multiple containers, iteration patterns.
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

  bool operator==(Iter other) {
    return p == other.p;
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

  int size() const {
    return len;
  }

  bool empty() const {
    return len == 0;
  }
};

int sum_all(IntArray* a) {
  int s = 0;
  for (Iter it = a->begin(); it != a->end(); ++it) {
    s = s + *it;
  }
  return s;
}

int main() {
  // Container 1
  IntArray a;
  a.buf[0] = 1;
  a.buf[1] = 2;
  a.buf[2] = 3;
  a.len = 3;

  if (a.size() != 3) return 1;
  if (a.empty()) return 2;
  if (sum_all(&a) != 6) return 3;

  // Container 2
  IntArray b;
  b.buf[0] = 10;
  b.buf[1] = 20;
  b.len = 2;

  if (sum_all(&b) != 30) return 4;

  // Empty container
  IntArray empty;
  empty.len = 0;
  if (!empty.empty()) return 5;
  if (sum_all(&empty) != 0) return 6;

  // Verify begin == end for empty
  Iter eb = empty.begin();
  Iter ee = empty.end();
  if (eb != ee) return 7;
  if (!(eb == ee)) return 8;

  return 0;
}
