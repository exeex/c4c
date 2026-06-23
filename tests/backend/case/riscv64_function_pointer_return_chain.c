typedef int (*BinaryFn)(int, int);

int sub(int left, int right) { return left - right; }

BinaryFn choose(int selector) {
  if (selector)
    return sub;
  return 0;
}

int main(void) {
  BinaryFn selected = choose(1);
  return selected(9, 4) - 5;
}
