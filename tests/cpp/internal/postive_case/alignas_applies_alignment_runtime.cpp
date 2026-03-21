// Runtime regression: alignas(symbol) / alignas(type-id) should affect alignof(T).
constexpr unsigned kBoxAlign = 16;

struct alignas(kBoxAlign) AlignedFromSymbol {
  int value;
};

struct alignas(unsigned __int128) AlignedFromType {
  int value;
};

struct alignas(sizeof(unsigned __int128)) AlignedFromSizeofType {
  int value;
};

int main() {
  if (alignof(AlignedFromSymbol) != kBoxAlign)
    return 1;
  if (alignof(AlignedFromType) != alignof(unsigned __int128))
    return 2;
  if (alignof(AlignedFromSizeofType) != sizeof(unsigned __int128))
    return 3;
  return 0;
}
