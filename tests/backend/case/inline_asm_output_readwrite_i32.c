int main(void) {
  int x = 7;
  __asm__ volatile("" : "+r"(x));
  return x;
}
