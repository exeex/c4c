int rv64_step4_static_initialized_counter(int value) {
  static int retained = 13;
  int prior = retained;
  retained = value;
  return prior;
}

int main(void) {
  int first = rv64_step4_static_initialized_counter(11);
  int second = rv64_step4_static_initialized_counter(23);
  return first + second;
}
