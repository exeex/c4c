int main(void) {
  int x = 7;
  int y = 9;
  __asm__ volatile("" : : "r"(x), "r"(y));
  return x + y;
}
