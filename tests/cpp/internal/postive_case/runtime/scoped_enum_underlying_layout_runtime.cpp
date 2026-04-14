// Runtime regression: explicit scoped-enum underlying types must control
// sizeof/alignof and aggregate layout.

enum class Tiny : unsigned char {
  A = 1,
  B = 2,
};

enum class Wide : unsigned long long {
  A = 1,
  B = 2,
};

struct TinyBox {
  Tiny value;
  unsigned char tail;
};

struct WideBox {
  Wide value;
  unsigned char tail;
};

int main() {
  TinyBox tiny{};
  WideBox wide{};

  if (sizeof(Tiny) != sizeof(unsigned char))
    return 1;
  if (alignof(Tiny) != alignof(unsigned char))
    return 2;
  if (sizeof(TinyBox) != 2)
    return 3;

  if (sizeof(Wide) != sizeof(unsigned long long))
    return 4;
  if (alignof(Wide) != alignof(unsigned long long))
    return 5;
  if (sizeof(WideBox) != 16)
    return 6;
  if ((unsigned long)((char*)&tiny.tail - (char*)&tiny.value) != 1)
    return 7;
  if ((unsigned long)((char*)&wide.tail - (char*)&wide.value) != 8)
    return 8;

  return 0;
}
