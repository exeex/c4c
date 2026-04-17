int main(void) {
  int x = 7;
  int *p = &x;
  asm volatile("" : : "r"(p));
  return *p;
}
