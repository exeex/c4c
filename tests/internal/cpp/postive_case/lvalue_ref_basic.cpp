int bump(int& value) {
  value = value + 2;
  return value;
}

int main() {
  int base = 3;
  int& ref = base;
  ref = 7;
  if (base != 7) return 1;
  if (bump(ref) != 9) return 2;
  if (base != 9) return 3;
  return 0;
}
