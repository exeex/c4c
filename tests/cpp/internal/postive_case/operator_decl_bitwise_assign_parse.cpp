// Parse-only regression: record members should accept bitwise compound
// assignment operator overloads.
// RUN: %c4cll --parse-only %s

struct Bits {
  int value = 0;

  Bits& operator&=(const Bits& rhs);
  Bits& operator|=(const Bits& rhs);
  Bits& operator^=(const Bits& rhs);
  Bits& operator<<=(int amount);
  Bits& operator>>=(int amount);
};

Bits& Bits::operator&=(const Bits& rhs) {
  value &= rhs.value;
  return *this;
}

Bits& Bits::operator|=(const Bits& rhs) {
  value |= rhs.value;
  return *this;
}

Bits& Bits::operator^=(const Bits& rhs) {
  value ^= rhs.value;
  return *this;
}

Bits& Bits::operator<<=(int amount) {
  value <<= amount;
  return *this;
}

Bits& Bits::operator>>=(int amount) {
  value >>= amount;
  return *this;
}

int main() {
  return 0;
}
