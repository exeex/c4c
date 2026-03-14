int f(int x) {
  return ({ int y = x + 1; y; });
}

int main(void) {
  int result = f(6);
  int expect = 7;
  if (result == expect) return 0;
  else return -1;
}
