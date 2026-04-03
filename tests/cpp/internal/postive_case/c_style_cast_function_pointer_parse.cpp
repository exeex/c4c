int add1(int x) { return x + 1; }

int main() {
  void* raw = 0;
  int (*fp)(int) = (int (*)(int))raw;
  int a = ((int (*)(int))&add1)(41);
  return fp == 0 ? a - 42 : 1;
}
