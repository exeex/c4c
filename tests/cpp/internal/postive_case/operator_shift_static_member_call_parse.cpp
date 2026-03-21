// Parse regression: unqualified static member call from an out-of-class method
// definition with an overloaded shift-expression argument.
struct S {
  int value;

  S() : value(0) {}
  S(int v) : value(v) {}

  static void F(const S& lhs, const S& rhs, S& out);
  void G(const S& rhs);
  S operator<<(int amount) const;
};

void S::F(const S& lhs, const S& rhs, S& out) {
  out.value = lhs.value + rhs.value;
}

S S::operator<<(int amount) const {
  return S(this->value << amount);
}

void S::G(const S& rhs) {
  S result;
  F(result, rhs << 3, result);
}

int main() {
  S box(2);
  box.G(box);
  return 0;
}
