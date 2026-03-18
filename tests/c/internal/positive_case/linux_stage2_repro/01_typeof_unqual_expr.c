unsigned long f(unsigned long *addr) {
  return (*(const volatile __typeof_unqual__(*(unsigned long *)addr) *)&(*(unsigned long *)addr));
}

int main(void) {
  unsigned long value = 7;
  unsigned long result = f(&value);
  unsigned long expect = 7;
  if (result == expect) return 0;
  else return -1;
}
