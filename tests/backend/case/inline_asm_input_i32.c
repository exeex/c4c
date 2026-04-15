int main(void) {
  int x = 7;
  asm volatile("" : : "r"(x));
  return x;
}
