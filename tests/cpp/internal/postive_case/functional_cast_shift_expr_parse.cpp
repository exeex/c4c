// Parse-only regression: speculative type parsing inside a parenthesized
// expression must not corrupt the later shift-expression token stream.
// RUN: %c4cll --parse-only %s

struct Size {
  Size() = default;
  explicit Size(int value) : bits(value) {}

  Size operator~() const { return Size(); }
  Size operator>>(const Size& rhs) const {
    Size out;
    out.bits = bits >> rhs.bits;
    return out;
  }
  Size& operator|=(const Size& rhs) {
    bits |= rhs.bits;
    return *this;
  }

  int bits = 0;
};

struct Diff {
  Size rep;

  void clamp(const Diff& other) {
    rep |= ~(Size(-1) >> other.rep);
  }
};

int main() {
  return 0;
}
