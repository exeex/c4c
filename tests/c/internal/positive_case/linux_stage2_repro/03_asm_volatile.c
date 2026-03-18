void cpu_relax(void) {
  asm volatile("yield" ::: "memory");
}

int main(void) {
  int result = 0;
  cpu_relax();
  int expect = 0;
  if (result == expect) return 0;
  else return -1;
}
