int plus1(int x) { return x + 1; }
int minus1(int x) { return x - 1; }

int (*pick(int which))(int) {
  return which ? plus1 : minus1;
}

int main(void) {
  if (pick(1)(41) != 42) return 1;
  if (pick(0)(41) != 40) return 2;
  return 0;
}
