#pragma pack(push, 1)
struct PackedPair {
  char tag;
  int xs[3];
};
#pragma pack(pop)

int main(void) {
  struct PackedPair p = {2, {4, 8, 11}};
  int i = 1;
  return p.xs[i];
}
