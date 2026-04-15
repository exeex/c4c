int main(void) {
  int x = 7;
  int y = 0;
  __asm__ volatile("" : "=r"(y) : "r"(x));
  return y;
}
