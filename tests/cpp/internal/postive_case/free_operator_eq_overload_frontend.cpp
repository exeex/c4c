// Regression: free operator== / operator!= overloads should form a C++
// overload set instead of conflicting like plain C functions.

struct Box {
  int value;
};

struct Flag {
  int value;
};

bool operator==(const Box& lhs, const Box& rhs) {
  return lhs.value == rhs.value;
}

bool operator==(const Flag& lhs, const Flag& rhs) {
  return lhs.value == rhs.value;
}

bool operator!=(const Box& lhs, const Box& rhs) {
  return !(lhs == rhs);
}

bool operator!=(const Flag& lhs, const Flag& rhs) {
  return !(lhs == rhs);
}

int main() {
  Box a{1};
  Box b{1};
  Flag f{2};
  Flag g{3};
  return (a == b) + (f != g);
}
