int read_local_address(int *ptr) {
  return *ptr;
}

int main(void) {
  int value;
  int result;

  value = 41;
  result = read_local_address(&value);
  return result - 41;
}
