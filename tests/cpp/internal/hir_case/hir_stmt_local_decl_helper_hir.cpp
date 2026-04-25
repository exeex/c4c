// HIR regression: local declaration lowering should keep default construction,
// copy initialization, aggregate array setup, and rvalue-reference temporaries
// stable across helper extraction from impl/stmt/stmt.cpp.

struct Box {
  int value;

  Box() { value = 4; }
  Box(const Box& other) { value = other.value + 1; }
};

int main() {
  Box seed;
  Box copy = seed;
  int data[3] = {1, 2, 3};
  int&& moved = 7;
  return copy.value + data[1] + moved;
}
