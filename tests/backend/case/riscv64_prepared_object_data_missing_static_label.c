int rv64_step5_static_counter(int value) {
  static int retained;
  int prior = retained;
  retained = value;
  return prior;
}

int main(void) {
  int first = rv64_step5_static_counter(11);
  int second = rv64_step5_static_counter(23);
  return first + second;
}
