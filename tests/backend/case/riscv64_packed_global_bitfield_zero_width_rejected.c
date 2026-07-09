#pragma pack(1)
struct Rv64PackedZeroWidthBits {
  int a : 4;
  int : 0;
  int b : 4;
};

struct Rv64PackedZeroWidthBits rv64_packed_zero_width_bits;

int main(void) {
  rv64_packed_zero_width_bits.a = 1;
  rv64_packed_zero_width_bits.b = 2;
  return rv64_packed_zero_width_bits.a + rv64_packed_zero_width_bits.b;
}
