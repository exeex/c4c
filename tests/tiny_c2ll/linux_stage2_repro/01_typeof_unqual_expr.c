unsigned long f(unsigned long *addr) {
  return (*(const volatile __typeof_unqual__(*(unsigned long *)addr) *)&(*(unsigned long *)addr));
}
