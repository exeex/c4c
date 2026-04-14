// Runtime test: const/non-const operator[] pair on a container-like struct.
struct Vec {
  int data[4];
  int sz;

  // Non-const version returns pointer for mutation
  int* operator[](int idx) {
    return &data[idx];
  }

  // Const version returns by value
  int operator[](int idx) const {
    return data[idx];
  }

  int size() const {
    return sz;
  }
};

int main() {
  Vec v;
  v.data[0] = 10;
  v.data[1] = 20;
  v.data[2] = 30;
  v.data[3] = 40;
  v.sz = 4;

  // non-const operator[] returns int*
  int* p = v[0];
  *p = 100;
  if (v.data[0] != 100) return 1;

  // const method size()
  if (v.size() != 4) return 2;

  return 0;
}
