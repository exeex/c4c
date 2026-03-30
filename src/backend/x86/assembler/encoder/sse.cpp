#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// SSE and SIMD instruction encoders.

namespace c4c::backend::x86::assembler::encoder {

void InstructionEncoder::encode_sse_rr_rm(const std::vector<Operand>& ops,
                                           const std::vector<std::uint8_t>& load_opcode,
                                           const std::vector<std::uint8_t>& store_opcode) {
  if (ops.size() != 2) return;
  const auto emit_prefix = [&](const std::vector<std::uint8_t>& opcode) {
    const auto prefix_len = std::find(opcode.begin(), opcode.end(), 0x0F) - opcode.begin();
    bytes.insert(bytes.end(), opcode.begin(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len));
    return prefix_len;
  };
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    const auto prefix_len = emit_prefix(load_opcode);
    emit_rex_rr(0, ops[1].reg.name, ops[0].reg.name);
    bytes.insert(bytes.end(), load_opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), load_opcode.end());
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    const auto prefix_len = emit_prefix(load_opcode);
    emit_rex_rm(0, ops[1].reg.name, ops[0].mem);
    bytes.insert(bytes.end(), load_opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), load_opcode.end());
    encode_modrm_mem(dst_num, ops[0].mem);
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto prefix_len = emit_prefix(store_opcode);
    emit_rex_rm(0, ops[0].reg.name, ops[1].mem);
    bytes.insert(bytes.end(), store_opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), store_opcode.end());
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_sse_op(const std::vector<Operand>& ops,
                                       const std::vector<std::uint8_t>& opcode) {
  if (ops.size() != 2) return;
  const auto use_mmx = (ops[0].kind == Operand::Kind::Register && is_mmx(ops[0].reg.name)) ||
                       (ops[1].kind == Operand::Kind::Register && is_mmx(ops[1].reg.name));
  const auto prefix_len = std::find(opcode.begin(), opcode.end(), 0x0F) - opcode.begin();
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    if (!use_mmx) {
      bytes.insert(bytes.end(), opcode.begin(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len));
      emit_rex_rr(0, ops[1].reg.name, ops[0].reg.name);
    }
    bytes.insert(bytes.end(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), opcode.end());
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    if (!use_mmx) {
      bytes.insert(bytes.end(), opcode.begin(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len));
      emit_rex_rm(0, ops[1].reg.name, ops[0].mem);
    }
    bytes.insert(bytes.end(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), opcode.end());
    encode_modrm_mem(dst_num, ops[0].mem);
  }
}

void InstructionEncoder::encode_sse_store(const std::vector<Operand>& ops,
                                          const std::vector<std::uint8_t>& opcode) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Register && ops[0].reg.name.rfind("xmm", 0) == 0 &&
      ops[1].kind == Operand::Kind::Memory) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto prefix_len = std::find(opcode.begin(), opcode.end(), 0x0F) - opcode.begin();
    bytes.insert(bytes.end(), opcode.begin(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len));
    emit_rex_rm(0, ops[0].reg.name, ops[1].mem);
    bytes.insert(bytes.end(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), opcode.end());
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_sse_op_imm8(const std::vector<Operand>& ops,
                                            const std::vector<std::uint8_t>& opcode) {
  if (ops.size() != 3) return;
  if (ops[0].kind == Operand::Kind::Immediate &&
      ops[1].kind == Operand::Kind::Register &&
      ops[2].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[1].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[2].reg.name).value_or(0);
    const auto prefix_len = std::find(opcode.begin(), opcode.end(), 0x0F) - opcode.begin();
    bytes.insert(bytes.end(), opcode.begin(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len));
    emit_rex_rr(0, ops[2].reg.name, ops[1].reg.name);
    bytes.insert(bytes.end(), opcode.begin() + static_cast<std::ptrdiff_t>(prefix_len), opcode.end());
    bytes.push_back(modrm(3, dst_num, src_num));
    bytes.push_back(static_cast<std::uint8_t>(ops[0].imm.integer));
  }
}

void InstructionEncoder::encode_movd(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register &&
      is_xmm(ops[1].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    bytes.push_back(0x66);
    emit_rex_rr(0, ops[1].reg.name, ops[0].reg.name);
    bytes.insert(bytes.end(), {0x0F, 0x6E});
    bytes.push_back(modrm(3, dst_num, src_num));
  }
}

void InstructionEncoder::encode_movnti(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto size = is_reg64(ops[0].reg.name) ? 8 : 4;
    emit_rex_rm(size, ops[0].reg.name, ops[1].mem);
    bytes.insert(bytes.end(), {0x0F, 0xC3});
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_movnti_q(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    emit_rex_rm(8, ops[0].reg.name, ops[1].mem);
    bytes.insert(bytes.end(), {0x0F, 0xC3});
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

}  // namespace c4c::backend::x86::assembler::encoder
