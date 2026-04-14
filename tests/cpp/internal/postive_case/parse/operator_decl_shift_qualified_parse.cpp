// Parse test: qualified out-of-class shift operator definitions.
struct ShiftBox {
  int value;

  ShiftBox operator<<(int amount) const;
  ShiftBox& operator>>=(int amount);
};

ShiftBox ShiftBox::operator<<(int amount) const {
  ShiftBox out;
  out.value = value << amount;
  return out;
}

ShiftBox& ShiftBox::operator>>=(int amount) {
  value = value >> amount;
  return *this;
}

int main() {
  return 0;
}
