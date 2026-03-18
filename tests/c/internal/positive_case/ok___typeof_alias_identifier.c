int f(void) {
  int x = 4;
  __typeof__(x) y = 5;
  return x + y;
}

int main(void) {
  int result = f();
  int expect = 9;
  if (result == expect) return 0;
  else return -1;
}
