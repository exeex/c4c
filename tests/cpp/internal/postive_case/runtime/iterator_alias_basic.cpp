// Phase 4: Nested type aliases — typedef inside struct + StructName::TypeName access.
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

struct Container {
  typedef Iter iterator;
  typedef int value_type;

  int buf[8];
  int len;

  iterator begin() {
    iterator it;
    it.p = buf;
    return it;
  }

  iterator end() {
    iterator it;
    it.p = buf + len;
    return it;
  }

  value_type get(int i) {
    return buf[i];
  }

  int size() const {
    return len;
  }
};

int main() {
  Container c;
  c.buf[0] = 10;
  c.buf[1] = 20;
  c.buf[2] = 30;
  c.len = 3;

  // Use scoped typedef: Container::iterator
  Container::iterator it = c.begin();
  if (*it != 10) return 1;
  ++it;
  if (*it != 20) return 2;

  // Use scoped typedef: Container::value_type
  Container::value_type v = c.get(0);
  if (v != 10) return 3;

  // Iterate with scoped type
  int sum = 0;
  for (Container::iterator i = c.begin(); i != c.end(); ++i) {
    sum = sum + *i;
  }
  if (sum != 60) return 4;

  // Internal use: methods use unqualified typedef names
  if (c.size() != 3) return 5;

  return 0;
}
