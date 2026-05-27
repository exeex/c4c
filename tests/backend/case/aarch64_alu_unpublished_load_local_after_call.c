extern int printf(const char *, ...);

int main(void) {
  int a = 12;
  int b = 34;
  int c = 56;
  int d = 78;

  printf("%d\n", c + d);

  int masked = c & d;
  int mixed = b ^ masked;
  int result = a | mixed;

  return result != 46;
}
