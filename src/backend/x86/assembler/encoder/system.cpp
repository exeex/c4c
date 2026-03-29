#include <cstdint>
#include <string>
#include <vector>

// System instruction encoders.

namespace c4c::backend::x86::assembler::encoder {

void InstructionEncoder::encode_out(const std::vector<Operand>& ops, const std::string& mnemonic) {
  const std::uint8_t size = mnemonic == "outb" ? 1 : (mnemonic == "outw" ? 2 : 4);
  if (ops.empty()) {
    if (size == 2) bytes.push_back(0x66);
    bytes.push_back(size == 1 ? 0xEE : 0xEF);
    return;
  }
  if (ops.size() != 2) return;
  if (size == 2) bytes.push_back(0x66);
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) {
    bytes.push_back(size == 1 ? 0xEE : 0xEF);
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory) {
    bytes.push_back(size == 1 ? 0xEE : 0xEF);
  } else if (ops[1].kind == Operand::Kind::Immediate) {
    bytes.push_back(size == 1 ? 0xE6 : 0xE7);
    bytes.push_back(static_cast<std::uint8_t>(ops[1].imm.integer));
  }
}

void InstructionEncoder::encode_in(const std::vector<Operand>& ops, const std::string& mnemonic) {
  const std::uint8_t size = mnemonic == "inb" ? 1 : (mnemonic == "inw" ? 2 : 4);
  if (ops.empty()) {
    if (size == 2) bytes.push_back(0x66);
    bytes.push_back(size == 1 ? 0xEC : 0xED);
    return;
  }
  if (ops.size() != 2) return;
  if (size == 2) bytes.push_back(0x66);
  if ((ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) ||
      (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register)) {
    bytes.push_back(size == 1 ? 0xEC : 0xED);
  } else if (ops[0].kind == Operand::Kind::Immediate) {
    bytes.push_back(size == 1 ? 0xE4 : 0xE5);
    bytes.push_back(static_cast<std::uint8_t>(ops[0].imm.integer));
  }
}

void InstructionEncoder::encode_clflush(const std::vector<Operand>& ops) {
  if (ops.size() != 1) return;
  if (ops[0].kind == Operand::Kind::Memory) {
    emit_rex_rm(0, "", ops[0].mem);
    bytes.push_back(0x0F);
    bytes.push_back(0xAE);
    encode_modrm_mem(7, ops[0].mem);
  }
}

void InstructionEncoder::encode_verw(const std::vector<Operand>& ops) {
  if (ops.size() != 1) return;
  if (ops[0].kind == Operand::Kind::Memory) {
    emit_rex_rm(0, "", ops[0].mem);
    bytes.push_back(0x0F);
    bytes.push_back(0x00);
    encode_modrm_mem(5, ops[0].mem);
  } else if (ops[0].kind == Operand::Kind::Register) {
    if (auto num = reg_num(ops[0].reg.name)) {
      bytes.push_back(0x0F);
      bytes.push_back(0x00);
      bytes.push_back(modrm(3, 5, *num));
    }
  }
}

void InstructionEncoder::encode_mov_cr(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind != Operand::Kind::Register || ops[1].kind != Operand::Kind::Register) return;
  if (!is_control_reg(ops[0].reg.name)) return;
  const auto cr_num = control_reg_num(ops[0].reg.name).value_or(0);
  const auto gp_num = reg_num(ops[1].reg.name).value_or(0);
  std::uint8_t rex = 0;
  if (cr_num >= 8) rex |= 0x44;
  if (needs_rex_ext(ops[1].reg.name)) rex |= 0x41;
  if (rex != 0) bytes.push_back(rex);
  bytes.push_back(0x0F);
  bytes.push_back(0x20);
  bytes.push_back(modrm(3, static_cast<std::uint8_t>(cr_num & 7), gp_num));
}

void InstructionEncoder::encode_mov_dr(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind != Operand::Kind::Register || ops[1].kind != Operand::Kind::Register) return;
  if (!is_debug_reg(ops[0].reg.name)) return;
  const auto dr_num = debug_reg_num(ops[0].reg.name).value_or(0);
  const auto gp_num = reg_num(ops[1].reg.name).value_or(0);
  std::uint8_t rex = 0;
  if (needs_rex_ext(ops[1].reg.name)) rex |= 0x41;
  if (rex != 0) bytes.push_back(rex);
  bytes.push_back(0x0F);
  bytes.push_back(0x21);
  bytes.push_back(modrm(3, dr_num, gp_num));
}

void InstructionEncoder::encode_system_table(const std::vector<Operand>& ops, const std::string& mnemonic) {
  if (ops.size() != 1) return;
  const auto base = mnemonic.substr(0, mnemonic.size() - 1);
  std::uint8_t reg_ext = 0;
  if (base == "sgdt") reg_ext = 0;
  else if (base == "sidt") reg_ext = 1;
  else if (base == "lgdt") reg_ext = 2;
  else if (base == "lidt") reg_ext = 3;
  if (ops[0].kind == Operand::Kind::Memory) {
    emit_rex_rm(0, "", ops[0].mem);
    bytes.push_back(0x0F);
    bytes.push_back(0x01);
    encode_modrm_mem(reg_ext, ops[0].mem);
  } else if (ops[0].kind == Operand::Kind::Register) {
  }
}

void InstructionEncoder::encode_lmsw(const std::vector<Operand>& ops) {
  if (ops.size() != 1) return;
  if (ops[0].kind == Operand::Kind::Register) {
    const auto rm = reg_num(ops[0].reg.name).value_or(0);
    if (needs_rex_ext(ops[0].reg.name)) bytes.push_back(rex(false, false, false, true));
    bytes.push_back(0x0F);
    bytes.push_back(0x01);
    bytes.push_back(modrm(3, 6, rm));
  } else if (ops[0].kind == Operand::Kind::Memory) {
    emit_rex_rm(0, "", ops[0].mem);
    bytes.push_back(0x0F);
    bytes.push_back(0x01);
    encode_modrm_mem(6, ops[0].mem);
  }
}

}  // namespace c4c::backend::x86::assembler::encoder
