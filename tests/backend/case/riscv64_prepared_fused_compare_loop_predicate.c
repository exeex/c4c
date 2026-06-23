int main(void) {
  int remaining = 3;
  int sum = 0;

  while (remaining != 0) {
    sum = sum + 2;
    remaining = remaining - 1;
  }

  return sum == 6 ? 0 : 1;
}
