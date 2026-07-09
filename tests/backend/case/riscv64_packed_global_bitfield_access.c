#pragma pack(1)
struct Rv64PackedGlobalBits {
  int b : 18;
  int c : 1;
  int d : 24;
  int e : 15;
  int f : 14;
};

struct Rv64PackedGlobalBits rv64_packed_global_left;
struct Rv64PackedGlobalBits rv64_packed_global_right;

void rv64_packed_global_seed(void) {
  rv64_packed_global_left.e = 0;
  rv64_packed_global_left.b = 5;
  rv64_packed_global_left.c = 0;
  rv64_packed_global_left.d = -5;
  rv64_packed_global_left.f = 5;

  rv64_packed_global_right.b = 5;
  rv64_packed_global_right.c = 0;
  rv64_packed_global_right.d = -5;
  rv64_packed_global_right.e = 0;
  rv64_packed_global_right.f = 5;
}

int main(void) {
  rv64_packed_global_seed();
  if (rv64_packed_global_left.b != rv64_packed_global_right.b ||
      rv64_packed_global_left.c != rv64_packed_global_right.c ||
      rv64_packed_global_left.d != rv64_packed_global_right.d ||
      rv64_packed_global_left.e != rv64_packed_global_right.e ||
      rv64_packed_global_left.f != rv64_packed_global_right.f) {
    return 1;
  }
  return 0;
}
