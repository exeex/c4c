constexpr int global_base = 10;

int main() {
  const int a = 3;
  const int b = a + 4;
  constexpr int c = 20;
  const int d = c + b + global_base;
  int result = d;
  int expect = 37;
  if (result == expect) return 0;
  else return -1;
}
