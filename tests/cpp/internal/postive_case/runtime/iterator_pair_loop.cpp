// Phase 3: Iterator pair loop — classic for(it=begin; it!=end; ++it) pattern.
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

struct Vec {
  int storage[5];
  int sz;

  Iter begin() {
    Iter it;
    it.p = storage;
    return it;
  }

  Iter end() {
    Iter it;
    it.p = storage + sz;
    return it;
  }
};

int main() {
  Vec v;
  v.storage[0] = 2;
  v.storage[1] = 4;
  v.storage[2] = 6;
  v.storage[3] = 8;
  v.storage[4] = 10;
  v.sz = 5;

  // Classic for loop with iterator pair
  int sum = 0;
  for (Iter it = v.begin(); it != v.end(); ++it) {
    sum = sum + *it;
  }
  // 2+4+6+8+10 = 30
  if (sum != 30) return 1;

  // Find max
  int mx = 0;
  for (Iter it = v.begin(); it != v.end(); ++it) {
    if (*it > mx) {
      mx = *it;
    }
  }
  if (mx != 10) return 2;

  // Count elements
  int cnt = 0;
  for (Iter it = v.begin(); it != v.end(); ++it) {
    cnt = cnt + 1;
  }
  if (cnt != 5) return 3;

  return 0;
}
