// Phase 3: Basic begin/end member functions returning iterators.
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

struct Array {
  int data[3];
  int count;

  Iter begin() {
    Iter it;
    it.p = data;
    return it;
  }

  Iter end() {
    Iter it;
    it.p = data + count;
    return it;
  }
};

int main() {
  Array a;
  a.data[0] = 5;
  a.data[1] = 10;
  a.data[2] = 15;
  a.count = 3;

  Iter b = a.begin();
  Iter e = a.end();

  if (*b != 5) return 1;
  if (!(b != e)) return 2;

  // Full traversal
  int sum = 0;
  Iter it = a.begin();
  while (it != a.end()) {
    sum = sum + *it;
    ++it;
  }
  if (sum != 30) return 3;

  return 0;
}
