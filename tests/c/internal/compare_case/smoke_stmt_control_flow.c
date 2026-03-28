// Compare-mode smoke test: control-flow dispatcher coverage.

int run(void) {
  int sum = 0;
  int i = 0;

  for (; i < 4; i++) {
    switch (i) {
    case 0:
      sum += 1;
      continue;
    case 1:
      sum += 2;
      break;
    case 2:
      goto tail;
    default:
      sum += 100;
      break;
    }

    do {
      sum += 3;
    } while (0);

    while (sum < 6) {
      sum += 4;
      break;
    }
  }

  switch (sum) {
  case 8 ... 9:
    sum += 5;
    break;
  default:
    sum += 1000;
    break;
  }

tail:
  return sum + i;
}

int main(void) {
  return run() == 13 ? 0 : 1;
}
