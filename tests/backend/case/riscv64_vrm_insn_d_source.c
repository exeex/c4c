int main(void) {
  __c4c_builtin_vrm2 vd;
  __c4c_builtin_vrm2 a;
  __c4c_builtin_vrm2 b;
  __c4c_builtin_vrm2 c;
  __asm__ __volatile__(".insn.d %4, %5, %0, %1, %2, %3, %6"
                       : "=VRM2"(vd)
                       : "VRM2"(a), "VRM2"(b), "VRM2"(c), "i"(0x0a),
                         "i"(0x0b), "i"(0x03));
  return 0;
}
