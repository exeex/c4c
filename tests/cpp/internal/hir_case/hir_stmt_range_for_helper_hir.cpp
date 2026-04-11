// HIR regression: range-for lowering should keep the synthesized iterator
// locals, iterator method calls, and loop-carried element binding stable
// across helper extraction from hir_stmt.cpp.

struct Iter {
  int* p;

  int operator*() const { return *p; }

  Iter operator++() {
    p = p + 1;
    return *this;
  }

  bool operator!=(Iter other) const { return p != other.p; }
};

struct Bag {
  int values[3];

  Iter begin() {
    Iter it;
    it.p = values;
    return it;
  }

  Iter end() {
    Iter it;
    it.p = values + 3;
    return it;
  }

  Iter begin() const {
    Iter it;
    it.p = (int*)values;
    return it;
  }

  Iter end() const {
    Iter it;
    it.p = (int*)values + 3;
    return it;
  }
};

int sum_const(const Bag& bag) {
  int total = 0;
  for (int value : bag) {
    total = total + value;
  }
  return total;
}

int main() {
  Bag bag{{4, 5, 6}};
  return sum_const(bag) - 15;
}
