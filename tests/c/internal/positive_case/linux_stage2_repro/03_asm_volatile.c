#if defined(__x86_64__) || defined(__i386__)
#define CPU_RELAX_MNEMONIC "pause"
#elif defined(__aarch64__) || defined(__arm__)
#define CPU_RELAX_MNEMONIC "yield"
#else
#define CPU_RELAX_MNEMONIC "nop"
#endif

void cpu_relax(void) {
  asm volatile(CPU_RELAX_MNEMONIC ::: "memory");
}

int main(void) {
  int result = 0;
  cpu_relax();
  int expect = 0;
  if (result == expect) return 0;
  else return -1;
}
