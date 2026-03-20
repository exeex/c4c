// Runtime test: explicit this->member in an out-of-class operator definition.
struct ShiftBox {
  int value;

  ShiftBox() : value(0) {}
  ShiftBox(int v) : value(v) {}

  ShiftBox operator<<(int amount) const;
};

ShiftBox ShiftBox::operator<<(int amount) const {
  ShiftBox out;
  out.value = this->value << amount;
  return out;
}

int main() {
  ShiftBox box(3);
  ShiftBox shifted = box << 4;
  if (shifted.value != 48)
    return 1;
  return 0;
}
