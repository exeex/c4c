// Compare-mode smoke test: for-loop condition-to-latch block lifecycle.

int run(void) {
  int sum = 0;
  int i = 0;
  int j = 0;

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

  for (;; j++) {
    if (j == 0) {
      sum += 20;
      continue;
    }
    if (j == 2) {
      break;
    }
    sum += j;
  }

  return sum + i + j;
}

int main(void) {
  return run() == 38 ? 0 : 1;
}
