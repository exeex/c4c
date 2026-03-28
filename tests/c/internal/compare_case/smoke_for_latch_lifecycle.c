// Compare-mode smoke test: for-loop condition-to-latch block lifecycle.

int run(void) {
  int sum = 0;
  int i = 0;

  for (; i < 3; i++) {
    if (i == 1) {
      sum += 10;
      continue;
    }
    sum += i;
  }

  for (; i < 3; i++) {
    sum += 100;
  }

  return sum + i;
}

int main(void) {
  return run() == 15 ? 0 : 1;
}
