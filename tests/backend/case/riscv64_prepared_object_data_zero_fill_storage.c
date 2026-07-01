struct rv64_step5_zero_payload {
  int first;
  int second;
};

struct rv64_step5_zero_payload rv64_step5_zero_payload;

int main(void) {
  rv64_step5_zero_payload.first = 11;
  rv64_step5_zero_payload.second = 22;
  return rv64_step5_zero_payload.second - rv64_step5_zero_payload.first;
}
