static int __attribute__((noinline)) keep_live(int value) {
  return value + 1;
}

int main(void) {
  int a = 12;
  int b = 34;
  int c = 56;
  int d = 78;

  int marker = keep_live(c + d);
  int masked = c & d;
  int mixed = b ^ masked;
  int result = a | mixed;

  return (result != 46) || (marker != 135);
}
