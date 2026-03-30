#include <cstdint>
#include <string>
#include <vector>

// x87 floating-point and miscellaneous instruction encoders.

namespace c4c::backend::x86::assembler::encoder {

void InstructionEncoder::encode_bt(const std::vector<Operand>& ops, const std::string& mnemonic) {
  if (ops.size() != 2) return;
  const auto size = mnemonic_size_suffix(mnemonic).value_or(8);
  const auto base = mnemonic.substr(0, mnemonic.size() - 1);
  const auto reg_opcode = base == "bt" ? 0xA3 : base == "bts" ? 0xAB : base == "btr" ? 0xB3 : 0xBB;
  const auto imm_ext = base == "bt" ? 4 : base == "bts" ? 5 : base == "btr" ? 6 : 7;
  if (ops[0].kind == Operand::Kind::Immediate && ops[1].kind == Operand::Kind::Register) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_rex_unary(size, ops[1].reg.name);
    bytes.push_back(0x0F);
    bytes.push_back(0xBA);
    bytes.push_back(modrm(3, imm_ext, dst_num));
    bytes.push_back(static_cast<std::uint8_t>(ops[0].imm.integer));
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_rex_rr(size, ops[0].reg.name, ops[1].reg.name);
    bytes.push_back(0x0F);
    bytes.push_back(reg_opcode);
    bytes.push_back(modrm(3, src_num, dst_num));
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    emit_rex_rm(size, ops[0].reg.name, ops[1].mem);
    bytes.push_back(0x0F);
    bytes.push_back(reg_opcode);
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_x87_mem(const std::vector<Operand>& ops,
                                        const std::vector<std::uint8_t>& opcode,
                                        std::uint8_t ext) {
  if (ops.size() != 1) return;
  if (ops[0].kind == Operand::Kind::Memory) {
    emit_rex_rm(0, "", ops[0].mem);
    bytes.insert(bytes.end(), opcode.begin(), opcode.end());
    encode_modrm_mem(ext, ops[0].mem);
  }
}

void InstructionEncoder::encode_fcomip(const std::vector<Operand>& ops) {
  if (ops.empty()) {
    bytes.insert(bytes.end(), {0xDF, 0xF1});
    return;
  }
  if (ops.size() == 2 && ops[0].kind == Operand::Kind::Register) {
    const auto n = std::stoi(ops[0].reg.name.substr(3, ops[0].reg.name.size() - 4));
    bytes.insert(bytes.end(), {0xDF, static_cast<std::uint8_t>(0xF0 + n)});
  }
}

void InstructionEncoder::encode_fucomi(const std::vector<Operand>& ops) {
  if (ops.empty()) {
    bytes.insert(bytes.end(), {0xDB, 0xE9});
    return;
  }
  if ((ops.size() == 1 || ops.size() == 2) && ops[0].kind == Operand::Kind::Register) {
    const auto n = std::stoi(ops[0].reg.name.substr(3, ops[0].reg.name.size() - 4));
    bytes.insert(bytes.end(), {0xDB, static_cast<std::uint8_t>(0xE8 + n)});
  }
}

void InstructionEncoder::encode_fucomip(const std::vector<Operand>& ops) {
  if (ops.empty()) {
    bytes.insert(bytes.end(), {0xDF, 0xE9});
    return;
  }
  if (ops.size() == 2 && ops[0].kind == Operand::Kind::Register) {
    const auto n = std::stoi(ops[0].reg.name.substr(3, ops[0].reg.name.size() - 4));
    bytes.insert(bytes.end(), {0xDF, static_cast<std::uint8_t>(0xE8 + n)});
  }
}

void InstructionEncoder::encode_fld_st(const std::vector<Operand>& ops) {
  if (ops.size() != 1 || ops[0].kind != Operand::Kind::Register) return;
  const auto n = std::stoi(ops[0].reg.name.substr(3, ops[0].reg.name.size() - 4));
  bytes.insert(bytes.end(), {0xD9, static_cast<std::uint8_t>(0xC0 + n)});
}

void InstructionEncoder::encode_fstp_st(const std::vector<Operand>& ops) {
  if (ops.size() != 1 || ops[0].kind != Operand::Kind::Register) return;
  const auto n = std::stoi(ops[0].reg.name.substr(3, ops[0].reg.name.size() - 4));
  bytes.insert(bytes.end(), {0xDD, static_cast<std::uint8_t>(0xD8 + n)});
}

void InstructionEncoder::encode_bit_scan(const std::vector<Operand>& ops,
                                         const std::string& mnemonic,
                                         std::uint8_t opcode2) {
  if (ops.size() != 2) return;
  const auto size = mnemonic_size_suffix(mnemonic).value_or(8);
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    if (size == 2) bytes.push_back(0x66);
    emit_rex_rr(size, ops[1].reg.name, ops[0].reg.name);
    bytes.push_back(0x0F);
    bytes.push_back(opcode2);
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    if (size == 2) bytes.push_back(0x66);
    emit_rex_rm(size, ops[1].reg.name, ops[0].mem);
    bytes.push_back(0x0F);
    bytes.push_back(opcode2);
    encode_modrm_mem(dst_num, ops[0].mem);
  }
}

void InstructionEncoder::encode_mov_seg(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register &&
      is_segment_reg(ops[0].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    if (is_reg64(ops[1].reg.name)) {
      emit_rex_unary(8, ops[1].reg.name);
    } else if (needs_rex_ext(ops[1].reg.name)) {
      bytes.push_back(rex(false, false, false, true));
    }
    bytes.push_back(0x8C);
    bytes.push_back(modrm(3, src_num, dst_num));
  }
}

}  // namespace c4c::backend::x86::assembler::encoder
