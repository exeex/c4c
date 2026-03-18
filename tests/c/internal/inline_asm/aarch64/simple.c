int main(void) {
  int in = 1;
  int out = 7;
  asm volatile("sub %w0, %w1, %w1" : "=r"(out) : "r"(in) : "cc");
  return out;
}
