// Runtime test: shift expressions inside call arguments and member operator<<.
static int combine(int lhs, int mid, int rhs) {
  return lhs + mid + rhs;
}

struct ShiftBox {
  int value;

  ShiftBox() : value(0) {}
  ShiftBox(int v) : value(v) {}

  ShiftBox operator<<(int amount) const {
    return ShiftBox(value << amount);
  }
};

int main() {
  ShiftBox box(2);
  int sum = combine(1, (box << 3).value, 4);
  if (sum != 21)
    return 1;

  ShiftBox shifted = box << 4;
  if (shifted.value != 32)
    return 2;

  return 0;
}
