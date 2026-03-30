#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// Register classification and suffix inference helpers.
// Register classification helpers for the x86-64 encoder.

namespace c4c::backend::x86::assembler::encoder {

std::optional<std::uint8_t> reg_num(const std::string& name) {
  if (name == "al" || name == "ax" || name == "eax" || name == "rax" ||
      name == "xmm0" || name == "st" || name == "st(0)" || name == "mm0" ||
      name == "es" || name == "ymm0") return 0;
  if (name == "cl" || name == "cx" || name == "ecx" || name == "rcx" ||
      name == "xmm1" || name == "st(1)" || name == "mm1" || name == "cs" ||
      name == "ymm1") return 1;
  if (name == "dl" || name == "dx" || name == "edx" || name == "rdx" ||
      name == "xmm2" || name == "st(2)" || name == "mm2" || name == "ss" ||
      name == "ymm2") return 2;
  if (name == "bl" || name == "bx" || name == "ebx" || name == "rbx" ||
      name == "xmm3" || name == "st(3)" || name == "mm3" || name == "ds" ||
      name == "ymm3") return 3;
  if (name == "ah" || name == "spl" || name == "sp" || name == "esp" ||
      name == "rsp" || name == "xmm4" || name == "st(4)" || name == "mm4" ||
      name == "fs" || name == "ymm4") return 4;
  if (name == "ch" || name == "bpl" || name == "bp" || name == "ebp" ||
      name == "rbp" || name == "xmm5" || name == "st(5)" || name == "mm5" ||
      name == "gs" || name == "ymm5") return 5;
  if (name == "dh" || name == "sil" || name == "si" || name == "esi" ||
      name == "rsi" || name == "xmm6" || name == "st(6)" || name == "mm6" ||
      name == "ymm6") return 6;
  if (name == "bh" || name == "dil" || name == "di" || name == "edi" ||
      name == "rdi" || name == "xmm7" || name == "st(7)" || name == "mm7" ||
      name == "ymm7") return 7;
  if (name == "r8b" || name == "r8w" || name == "r8d" || name == "r8" ||
      name == "xmm8" || name == "ymm8") return 0;
  if (name == "r9b" || name == "r9w" || name == "r9d" || name == "r9" ||
      name == "xmm9" || name == "ymm9") return 1;
  if (name == "r10b" || name == "r10w" || name == "r10d" || name == "r10" ||
      name == "xmm10" || name == "ymm10") return 2;
  if (name == "r11b" || name == "r11w" || name == "r11d" || name == "r11" ||
      name == "xmm11" || name == "ymm11") return 3;
  if (name == "r12b" || name == "r12w" || name == "r12d" || name == "r12" ||
      name == "xmm12" || name == "ymm12") return 4;
  if (name == "r13b" || name == "r13w" || name == "r13d" || name == "r13" ||
      name == "xmm13" || name == "ymm13") return 5;
  if (name == "r14b" || name == "r14w" || name == "r14d" || name == "r14" ||
      name == "xmm14" || name == "ymm14") return 6;
  if (name == "r15b" || name == "r15w" || name == "r15d" || name == "r15" ||
      name == "xmm15" || name == "ymm15") return 7;
  return std::nullopt;
}

bool is_mmx(const std::string& name) {
  return name.rfind("mm", 0) == 0 && name.rfind("mmx", 0) != 0 &&
         name.size() <= 3 && name.size() > 2 &&
         static_cast<unsigned char>(name[2]) >= '0' &&
         static_cast<unsigned char>(name[2]) <= '9';
}

bool is_segment_reg(const std::string& name) {
  return name == "es" || name == "cs" || name == "ss" ||
         name == "ds" || name == "fs" || name == "gs";
}

bool is_control_reg(const std::string& name) {
  return name == "cr0" || name == "cr2" || name == "cr3" ||
         name == "cr4" || name == "cr8";
}

std::optional<std::uint8_t> control_reg_num(const std::string& name) {
  if (name == "cr0") return 0;
  if (name == "cr2") return 2;
  if (name == "cr3") return 3;
  if (name == "cr4") return 4;
  if (name == "cr8") return 8;
  return std::nullopt;
}

bool is_debug_reg(const std::string& name) {
  return name == "db0" || name == "db1" || name == "db2" || name == "db3" ||
         name == "db4" || name == "db5" || name == "db6" || name == "db7" ||
         name == "dr0" || name == "dr1" || name == "dr2" || name == "dr3" ||
         name == "dr4" || name == "dr5" || name == "dr6" || name == "dr7";
}

std::optional<std::uint8_t> debug_reg_num(const std::string& name) {
  if (name == "db0" || name == "dr0") return 0;
  if (name == "db1" || name == "dr1") return 1;
  if (name == "db2" || name == "dr2") return 2;
  if (name == "db3" || name == "dr3") return 3;
  if (name == "db4" || name == "dr4") return 4;
  if (name == "db5" || name == "dr5") return 5;
  if (name == "db6" || name == "dr6") return 6;
  if (name == "db7" || name == "dr7") return 7;
  return std::nullopt;
}

bool is_ymm(const std::string& name) {
  return name.rfind("ymm", 0) == 0;
}

bool needs_rex_ext(const std::string& name) {
  return name.rfind("r8", 0) == 0 || name.rfind("r9", 0) == 0 ||
         name.rfind("r10", 0) == 0 || name.rfind("r11", 0) == 0 ||
         name.rfind("r12", 0) == 0 || name.rfind("r13", 0) == 0 ||
         name.rfind("r14", 0) == 0 || name.rfind("r15", 0) == 0 ||
         name.rfind("xmm8", 0) == 0 || name.rfind("xmm9", 0) == 0 ||
         name.rfind("xmm10", 0) == 0 || name.rfind("xmm11", 0) == 0 ||
         name.rfind("xmm12", 0) == 0 || name.rfind("xmm13", 0) == 0 ||
         name.rfind("xmm14", 0) == 0 || name.rfind("xmm15", 0) == 0 ||
         name.rfind("ymm8", 0) == 0 || name.rfind("ymm9", 0) == 0 ||
         name.rfind("ymm10", 0) == 0 || name.rfind("ymm11", 0) == 0 ||
         name.rfind("ymm12", 0) == 0 || name.rfind("ymm13", 0) == 0 ||
         name.rfind("ymm14", 0) == 0 || name.rfind("ymm15", 0) == 0;
}

bool needs_vex_ext(const std::string& name) { return needs_rex_ext(name); }

bool is_reg64(const std::string& name) {
  return name == "rax" || name == "rcx" || name == "rdx" || name == "rbx" ||
         name == "rsp" || name == "rbp" || name == "rsi" || name == "rdi" ||
         name == "r8" || name == "r9" || name == "r10" || name == "r11" ||
         name == "r12" || name == "r13" || name == "r14" || name == "r15";
}

bool is_reg32(const std::string& name) {
  return name == "eax" || name == "ecx" || name == "edx" || name == "ebx" ||
         name == "esp" || name == "ebp" || name == "esi" || name == "edi" ||
         name == "r8d" || name == "r9d" || name == "r10d" || name == "r11d" ||
         name == "r12d" || name == "r13d" || name == "r14d" || name == "r15d";
}

bool is_reg16(const std::string& name) {
  return name == "ax" || name == "cx" || name == "dx" || name == "bx" ||
         name == "sp" || name == "bp" || name == "si" || name == "di" ||
         name == "r8w" || name == "r9w" || name == "r10w" || name == "r11w" ||
         name == "r12w" || name == "r13w" || name == "r14w" || name == "r15w";
}

bool is_reg8(const std::string& name) {
  return name == "al" || name == "cl" || name == "dl" || name == "bl" ||
         name == "ah" || name == "ch" || name == "dh" || name == "bh" ||
         name == "spl" || name == "bpl" || name == "sil" || name == "dil" ||
         name == "r8b" || name == "r9b" || name == "r10b" || name == "r11b" ||
         name == "r12b" || name == "r13b" || name == "r14b" || name == "r15b";
}

bool is_rex_required_8bit(const std::string& name) {
  return name == "spl" || name == "bpl" || name == "sil" || name == "dil";
}

bool is_xmm(const std::string& name) { return name.rfind("xmm", 0) == 0; }

bool is_xmm_or_ymm(const std::string& name) {
  return name.rfind("xmm", 0) == 0 || name.rfind("ymm", 0) == 0;
}

std::optional<char> register_size_suffix(const std::string& name) {
  if (is_reg64(name)) return 'q';
  if (is_reg32(name)) return 'l';
  if (is_reg16(name)) return 'w';
  if (is_reg8(name)) return 'b';
  return std::nullopt;
}

static constexpr const char* SUFFIXABLE_MNEMONICS[] = {
    "mov", "add", "sub", "and", "or", "xor", "cmp", "test",
    "push", "pop", "lea",
    "shl", "shr", "sar", "rol", "ror",
    "inc", "dec", "neg", "not",
    "imul", "mul", "div", "idiv",
    "adc", "sbb",
    "xchg", "cmpxchg", "xadd", "bswap",
    "bsf", "bsr",
};

std::string infer_suffix(const std::string& mnemonic, const std::vector<Operand>& ops) {
  for (const char* suffixable : SUFFIXABLE_MNEMONICS) {
    if (mnemonic == suffixable) {
      const bool is_shift = mnemonic == "shl" || mnemonic == "shr" ||
                            mnemonic == "sar" || mnemonic == "rol" ||
                            mnemonic == "ror";
      if (is_shift && ops.size() == 2) {
        if (ops[1].kind == Operand::Kind::Register) {
          if (auto suffix = register_size_suffix(ops[1].reg.name)) {
            return mnemonic + std::string(1, *suffix);
          }
        }
      }
      for (const auto& op : ops) {
        if (op.kind == Operand::Kind::Register) {
          if (auto suffix = register_size_suffix(op.reg.name)) {
            return mnemonic + std::string(1, *suffix);
          }
        }
      }
      return mnemonic;
    }
  }
  return mnemonic;
}

std::optional<std::uint8_t> mnemonic_size_suffix(const std::string& mnemonic) {
  if (mnemonic == "cltq" || mnemonic == "cqto" || mnemonic == "cltd" ||
      mnemonic == "cdq" || mnemonic == "cqo" || mnemonic == "ret" ||
      mnemonic == "nop" || mnemonic == "ud2" || mnemonic == "endbr64" ||
      mnemonic == "pause" || mnemonic == "mfence" || mnemonic == "lfence" ||
      mnemonic == "sfence" || mnemonic == "clflush" || mnemonic == "syscall" ||
      mnemonic == "sysenter" || mnemonic == "cpuid" || mnemonic == "rdtsc" ||
      mnemonic == "rdtscp" || mnemonic == "rdpmc" || mnemonic == "clc" ||
      mnemonic == "stc" || mnemonic == "cli" || mnemonic == "sti" ||
      mnemonic == "cld" || mnemonic == "std" || mnemonic == "sahf" ||
      mnemonic == "lahf" || mnemonic == "fninit" || mnemonic == "fwait" ||
      mnemonic == "wait" || mnemonic == "fnstcw" || mnemonic == "fstcw") {
    return std::nullopt;
  }
  if (!mnemonic.empty()) {
    const char suffix = mnemonic.back();
    if (suffix == 'q') return 8;
    if (suffix == 'l') return 4;
    if (suffix == 'w') return 2;
    if (suffix == 'b') return 1;
    }
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86::assembler::encoder
