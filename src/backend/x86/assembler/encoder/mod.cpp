#include "mod.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// x86-64 instruction encoder core declarations and relocation constants.

namespace c4c::backend::x86::assembler::encoder {

// Shared encoder helpers are implemented in sibling translation units.

struct Relocation {
  std::uint64_t offset = 0;
  std::string symbol;
  std::uint32_t reloc_type = 0;
  std::int64_t addend = 0;
};

// ELF x86-64 relocation types used by the encoder and object writer.
static constexpr std::uint32_t R_X86_64_NONE = 0;
static constexpr std::uint32_t R_X86_64_64 = 1;
static constexpr std::uint32_t R_X86_64_PC32 = 2;
static constexpr std::uint32_t R_X86_64_GOT32 = 3;
static constexpr std::uint32_t R_X86_64_PLT32 = 4;
static constexpr std::uint32_t R_X86_64_GOTPCREL = 9;
static constexpr std::uint32_t R_X86_64_32 = 10;
static constexpr std::uint32_t R_X86_64_32S = 11;
static constexpr std::uint32_t R_X86_64_16 = 12;
static constexpr std::uint32_t R_X86_64_TPOFF64 = 18;
static constexpr std::uint32_t R_X86_64_GOTTPOFF = 22;
static constexpr std::uint32_t R_X86_64_TPOFF32 = 23;
static constexpr std::uint32_t R_X86_64_PC8_INTERNAL = 0x8000'0001;

class InstructionEncoder {
 public:
  std::vector<std::uint8_t> bytes;
  std::vector<Relocation> relocations;
  std::uint64_t offset = 0;

  InstructionEncoder() = default;

  void encode(const Instruction& instr);
  void encode_mnemonic(const Instruction& instr);

  std::uint8_t rex(bool w, bool r, bool x, bool b) const;
  std::uint8_t modrm(std::uint8_t mod_, std::uint8_t reg, std::uint8_t rm) const;
  std::uint8_t sib(std::uint8_t scale, std::uint8_t index, std::uint8_t base) const;

  void emit_rex_rr(std::uint8_t size, const std::string& reg, const std::string& rm);
  void emit_segment_prefix(const MemoryOperand& mem);
  void emit_rex_rm(std::uint8_t size, const std::string& reg, const MemoryOperand& mem);
  void emit_rex_unary(std::uint8_t size, const std::string& rm);
  void encode_modrm_mem(std::uint8_t reg_field, const MemoryOperand& mem);
  void add_relocation(const std::string& symbol, std::uint32_t reloc_type, std::int64_t addend);
  void adjust_rip_reloc_addend(std::size_t reloc_count_before, std::int64_t trailing_bytes);

  // Instruction-family helpers translated in sibling .cpp files.
  void encode_mov(const std::vector<Operand>& ops, std::uint8_t size);
  void encode_mov_imm_reg(const ImmediateValue& imm, const Register& dst, std::uint8_t size);
  void encode_mov_rr(const Register& src, const Register& dst, std::uint8_t size);
  void encode_mov_mem_reg(const MemoryOperand& mem, const Register& dst, std::uint8_t size);
  void encode_mov_reg_mem(const Register& src, const MemoryOperand& mem, std::uint8_t size);
  void encode_mov_imm_mem(const ImmediateValue& imm, const MemoryOperand& mem, std::uint8_t size);
  void encode_movabs(const std::vector<Operand>& ops);
  void encode_movsx(const std::vector<Operand>& ops, std::uint8_t src_size, std::uint8_t dst_size);
  void encode_movzx(const std::vector<Operand>& ops, std::uint8_t src_size, std::uint8_t dst_size);
  void encode_lea(const std::vector<Operand>& ops, std::uint8_t size);
  void encode_push(const std::vector<Operand>& ops);
  void encode_pop(const std::vector<Operand>& ops);
  void encode_alu(const std::vector<Operand>& ops, const std::string& mnemonic, std::uint8_t ext);
  void encode_test(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_imul(const std::vector<Operand>& ops, std::uint8_t size);
  void encode_unary_rm(const std::vector<Operand>& ops, std::uint8_t ext, std::uint8_t size);
  void encode_inc_dec(const std::vector<Operand>& ops, std::uint8_t ext, std::uint8_t size);
  void encode_shift(const std::vector<Operand>& ops, const std::string& mnemonic, std::uint8_t ext);
  void encode_double_shift(const std::vector<Operand>& ops, std::uint8_t opcode2, std::uint8_t size);
  void encode_bswap(const std::vector<Operand>& ops, std::uint8_t size);
  void encode_bit_count(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_bsf_bsr(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_bt(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_setcc(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_cmovcc(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_jmp(const std::vector<Operand>& ops);
  void encode_ljmp(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_jcc(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_call(const std::vector<Operand>& ops);
  void encode_out(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_in(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_xchg(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_cmpxchg(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_xadd(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_sse_rr_rm(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& load_opcode, const std::vector<std::uint8_t>& store_opcode);
  void encode_sse_op(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_sse_store(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_sse_op_imm8(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_movd(const std::vector<Operand>& ops);
  void encode_movnti(const std::vector<Operand>& ops);
  void encode_movnti_q(const std::vector<Operand>& ops);
  void encode_rdrand(const std::vector<Operand>& ops, const std::string& mnemonic, std::uint8_t opcode2, std::uint8_t reg_ext);
  void encode_sldt(const std::vector<Operand>& ops);
  void encode_str_insn(const std::vector<Operand>& ops);
  void encode_crc32(const std::vector<Operand>& ops, std::uint8_t size);
  void encode_x87_mem(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode, std::uint8_t ext);
  void encode_fcomip(const std::vector<Operand>& ops);
  void encode_fucomi(const std::vector<Operand>& ops);
  void encode_fucomip(const std::vector<Operand>& ops);
  void encode_fld_st(const std::vector<Operand>& ops);
  void encode_fstp_st(const std::vector<Operand>& ops);
  void encode_bit_scan(const std::vector<Operand>& ops, const std::string& mnemonic, std::uint8_t opcode2);
  void encode_mov_seg(const std::vector<Operand>& ops);
  void encode_mov_cr(const std::vector<Operand>& ops);
  void encode_mov_dr(const std::vector<Operand>& ops);
  void encode_system_table(const std::vector<Operand>& ops, const std::string& mnemonic);
  void encode_lmsw(const std::vector<Operand>& ops);
  void encode_verw(const std::vector<Operand>& ops);
  void encode_clflush(const std::vector<Operand>& ops);
  void encode_avx_mov(const std::vector<Operand>& ops, std::uint8_t load_op, std::uint8_t store_op, bool is_66);
  void encode_avx_mov_np(const std::vector<Operand>& ops, std::uint8_t load_op, std::uint8_t store_op, bool is_66);
  void encode_avx_3op(const std::vector<Operand>& ops, std::uint8_t opcode, bool has_66);
  void encode_avx_broadcast(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_avx_insert(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_avx_extract(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode);
  void encode_avx_shift_imm(const std::vector<Operand>& ops, const std::vector<std::uint8_t>& opcode, std::uint8_t ext);
  void encode_avx_rotate_imm(const std::vector<Operand>& ops, std::uint8_t opcode, std::uint8_t ext, std::uint8_t w);
  void emit_vex(bool r, bool x, bool b, std::uint8_t mm, std::uint8_t w, std::uint8_t vvvv, std::uint8_t l, std::uint8_t pp);
};

}  // namespace c4c::backend::x86::assembler::encoder
