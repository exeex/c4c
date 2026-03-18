int f(void) {
  typeof(int) x = 3;
  return x;
}

int main(void) {
  int result = f();
  int expect = 3;
  if (result == expect) return 0;
  else return -1;
}
