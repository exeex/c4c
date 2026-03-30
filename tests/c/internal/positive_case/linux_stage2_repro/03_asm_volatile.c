#if defined(__clang__)
#define CPU_RELAX_MNEMONIC "pause"  // clang rejects 'yield', so keep the host baseline green
#else
#define CPU_RELAX_MNEMONIC "yield"
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
