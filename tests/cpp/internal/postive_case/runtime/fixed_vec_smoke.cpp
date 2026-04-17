// Milestone 3: Small vector-like usability smoke test.
// Exercises size/empty/data/front/back/operator[]/push_back/pop_back/begin/end/range-for.

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

struct FixedVec {
  int buf[16];
  int len;

  int size() const {
    return len;
  }

  bool empty() const {
    return len == 0;
  }

  int* data() {
    return buf;
  }

  int front() const {
    return buf[0];
  }

  int back() const {
    return buf[len - 1];
  }

  int* operator[](int idx) {
    return &buf[idx];
  }

  int operator[](int idx) const {
    return buf[idx];
  }

  void push_back(int val) {
    buf[len] = val;
    len = len + 1;
  }

  void pop_back() {
    len = len - 1;
  }

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

int sum_manual(FixedVec* v) {
  int s = 0;
  for (Iter it = v->begin(); it != v->end(); ++it) {
    s = s + *it;
  }
  return s;
}

int sum_range(FixedVec* v) {
  int s = 0;
  for (auto x : *v) {
    s = s + x;
  }
  return s;
}

int main() {
  FixedVec v;
  v.len = 0;

  // === empty state ===
  if (!v.empty()) return 1;
  if (v.size() != 0) return 2;

  // === push_back ===
  v.push_back(10);
  v.push_back(20);
  v.push_back(30);

  if (v.size() != 3) return 3;
  if (v.empty()) return 4;

  // === front / back ===
  if (v.front() != 10) return 5;
  if (v.back() != 30) return 6;

  // === data() ===
  int* d = v.data();
  if (d[0] != 10) return 7;
  if (d[1] != 20) return 8;
  if (d[2] != 30) return 9;

  // === operator[] non-const (returns int*) ===
  int* p = v[1];
  if (*p != 20) return 10;
  *p = 25;
  if (v.buf[1] != 25) return 11;

  // === manual iterator loop ===
  if (sum_manual(&v) != (10 + 25 + 30)) return 12;

  // === range-for loop ===
  if (sum_range(&v) != (10 + 25 + 30)) return 13;

  // === pop_back ===
  v.pop_back();
  if (v.size() != 2) return 14;
  if (v.back() != 25) return 15;

  // === push more, verify growth ===
  v.push_back(40);
  v.push_back(50);
  v.push_back(60);
  if (v.size() != 5) return 16;
  if (v.back() != 60) return 17;

  // === sum after mutations ===
  int expected = 10 + 25 + 40 + 50 + 60;
  if (sum_manual(&v) != expected) return 18;
  if (sum_range(&v) != expected) return 19;

  // === empty after pop_back all ===
  while (!v.empty()) {
    v.pop_back();
  }
  if (v.size() != 0) return 20;
  if (!v.empty()) return 21;
  if (sum_manual(&v) != 0) return 22;
  if (sum_range(&v) != 0) return 23;

  return 0;
}
