int main(void) {
  int x = 7;
  int *p = &x;
  int *q;
  __asm__ volatile("" : "=r"(q) : "r"(p));
  return 0;
}
