int rv64_variadic_entry_missing_contract(int seed, ...) {
  return seed;
}

int main(void) {
  return rv64_variadic_entry_missing_contract(1, 2);
}
