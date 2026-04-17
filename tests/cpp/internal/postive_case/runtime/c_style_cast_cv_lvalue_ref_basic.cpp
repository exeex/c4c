int main() {
  int x = 3;

  const int& as_const = (const int&)x;
  if (as_const != 3) return 1;

  volatile int& as_volatile = (volatile int&)x;
  as_volatile = 8;

  return x == 8 ? 0 : 2;
}
