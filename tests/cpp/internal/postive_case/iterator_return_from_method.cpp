// Phase 1: Returning iterator objects from container methods.
struct Iter {
  int* p;

  int operator*() {
    return *p;
  }
};

struct Box {
  int data[2];

  Iter first() {
    Iter it;
    it.p = data;
    return it;
  }

  Iter second() {
    Iter it;
    it.p = data + 1;
    return it;
  }
};

int main() {
  Box b;
  b.data[0] = 42;
  b.data[1] = 99;

  Iter a = b.first();
  if (*a != 42) return 1;

  Iter s = b.second();
  if (*s != 99) return 2;

  return 0;
}
