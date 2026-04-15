int main(void) {
  int x = 7;
  int *p = &x;
  __asm__ volatile("" : "+r"(p));
  return 0;
}
