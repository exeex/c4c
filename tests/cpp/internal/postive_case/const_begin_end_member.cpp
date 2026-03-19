// Phase 3: Const begin/end members on a container.
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

struct List {
  int elems[3];
  int n;

  Iter begin() const {
    Iter it;
    it.p = (int*)elems;
    return it;
  }

  Iter end() const {
    Iter it;
    it.p = (int*)elems + n;
    return it;
  }

  int size() const {
    return n;
  }
};

int main() {
  List l;
  l.elems[0] = 7;
  l.elems[1] = 14;
  l.elems[2] = 21;
  l.n = 3;

  if (l.size() != 3) return 1;

  // Use const begin/end
  int sum = 0;
  Iter it = l.begin();
  while (it != l.end()) {
    sum = sum + *it;
    ++it;
  }
  if (sum != 42) return 2;

  return 0;
}
