// Runtime test: operator== and operator!= member calls on a struct.
// Uses int parameter to avoid struct-by-value ABI complexity.
struct Val {
  int x;

  bool operator==(int other_x) {
    return x == other_x;
  }

  bool operator!=(int other_x) {
    return x != other_x;
  }
};

int main() {
  Val a;
  a.x = 10;

  if (!(a == 10)) return 1;
  if (a != 10) return 2;
  if (a == 20) return 3;
  if (!(a != 20)) return 4;
  return 0;
}
