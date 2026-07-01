struct rv64_step5_payload {
  int left;
  unsigned char bytes[4];
  int right;
};

struct rv64_step5_payload rv64_step5_initialized_payload = {
    5, {9, 8, 7, 6}, 17};

int main(void) {
  return rv64_step5_initialized_payload.left +
         rv64_step5_initialized_payload.bytes[2] +
         rv64_step5_initialized_payload.right;
}
