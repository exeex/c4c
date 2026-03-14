int f(void) {
  long x = 1;
  typeof(x) y = 2;
  return (int)(x + y);
}

int main(void) {
  int result = f();
  int expect = 3;
  if (result == expect) return 0;
  else return -1;
}
