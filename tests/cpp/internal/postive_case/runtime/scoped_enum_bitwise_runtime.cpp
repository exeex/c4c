// Regression: scoped enums should stay enums through parsing and codegen, even
// when used in bitwise helper functions similar to libstdc++'s `std::byte`
// operators.

enum class Byte : unsigned char {
  A = 1,
  B = 2,
};

Byte combine(Byte lhs, Byte rhs) {
  return (Byte)((unsigned char)lhs | (unsigned char)rhs);
}

int main() {
  Byte value = combine((Byte)1, (Byte)2);
  return (unsigned char)value == 3 ? 0 : 1;
}
