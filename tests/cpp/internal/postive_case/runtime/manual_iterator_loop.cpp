// Integration test: manual iterator loop combining all operator overloads.
// This validates the full iterator pattern needed for STL-like code.
struct Iter {
  int* ptr;

  int operator*() {
    return *ptr;
  }

  Iter operator++() {
    ptr = ptr + 1;
    Iter self;
    self.ptr = ptr;
    return self;
  }

  bool operator!=(Iter other) {
    return ptr != other.ptr;
  }
};

struct Container {
  int data[5];
  int sz;

  Iter begin() {
    Iter it;
    it.ptr = data;
    return it;
  }

  Iter end() {
    Iter it;
    it.ptr = data + sz;
    return it;
  }

  int size() const {
    return sz;
  }
};

int main() {
  Container c;
  c.data[0] = 10;
  c.data[1] = 20;
  c.data[2] = 30;
  c.data[3] = 40;
  c.data[4] = 50;
  c.sz = 5;

  if (c.size() != 5) return 1;

  // Manual iterator loop: for (auto it = c.begin(); it != c.end(); ++it)
  int sum = 0;
  Iter it = c.begin();
  Iter e = c.end();
  while (it != e) {
    sum = sum + *it;
    ++it;
  }

  // 10 + 20 + 30 + 40 + 50 = 150
  if (sum != 150) return 2;

  // Verify individual elements via iteration
  it = c.begin();
  if (*it != 10) return 3;
  ++it;
  if (*it != 20) return 4;
  ++it;
  if (*it != 30) return 5;
  ++it;
  if (*it != 40) return 6;
  ++it;
  if (*it != 50) return 7;

  return 0;
}
