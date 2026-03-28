// Compare-mode smoke test: chained case-range successor opening.

int classify(int x) {
  switch (x) {
  case 1 ... 2:
    return 10;
  case 4 ... 5:
    return 20;
  case 7 ... 9:
    return 30;
  default:
    return 40;
  }
}

int main(void) {
  return classify(8) == 30 &&
                 classify(4) == 20 &&
                 classify(3) == 40
             ? 0
             : 1;
}
