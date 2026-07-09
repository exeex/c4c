#pragma pack(1)
struct Rv64PackedTinyBits {
  int a : 1;
};

struct Rv64PackedTinyBits rv64_packed_tiny_bits;

int main(void) {
  rv64_packed_tiny_bits.a = 1;
  return rv64_packed_tiny_bits.a;
}
