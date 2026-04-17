// Runtime smoke test for extended member operator overload support.
struct Acc {
  int x;

  Acc() : x(0) {}
  Acc(int v) : x(v) {}

  Acc operator%(int rhs) const {
    Acc out;
    out.x = x % rhs;
    return out;
  }
  Acc& operator%=(int rhs) { x %= rhs; return *this; }
  Acc operator*(int rhs) const {
    Acc out;
    out.x = x * rhs;
    return out;
  }
  Acc operator~() const {
    Acc out;
    out.x = ~x;
    return out;
  }
  bool operator!() const { return x == 0; }
  Acc& operator--() { --x; return *this; }
  Acc operator--(int) {
    Acc prev;
    prev.x = x;
    x--;
    return prev;
  }
};

int main() {
  Acc a(19);
  Acc mod = a % 5;
  if (mod.x != 4)
    return 1;

  a %= 6;
  if (a.x != 1)
    return 2;

  Acc mul = a * 9;
  if (mul.x != 9)
    return 3;

  Acc zero(0);
  Acc bits = ~zero;
  if (bits.x != -1)
    return 4;

  Acc nonzero(3);
  if (!nonzero)
    return 5;
  if (!!zero)
    return 6;

  Acc dec(7);
  Acc before = dec--;
  if (before.x != 7 || dec.x != 6)
    return 7;

  --dec;
  if (dec.x != 5)
    return 8;

  return 0;
}
