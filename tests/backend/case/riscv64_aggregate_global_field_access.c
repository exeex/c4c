struct rv64_step4_pair {
  int left;
  int right;
};

struct rv64_step4_pair rv64_step4_pair_global;

int main(void) {
  rv64_step4_pair_global.left = 13;
  rv64_step4_pair_global.right = 29;
  return rv64_step4_pair_global.right - rv64_step4_pair_global.left;
}
