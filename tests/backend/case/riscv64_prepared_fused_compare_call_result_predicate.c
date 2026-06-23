int rv64_predicate_value(void) {
  return 0;
}

int main(void) {
  if (rv64_predicate_value() != 0) {
    return 1;
  }

  return 0;
}
