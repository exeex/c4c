#include <cstdint>
#include <string>
#include <vector>

// AVX instruction encoders.

namespace c4c::backend::x86::assembler::encoder {

void InstructionEncoder::encode_avx_mov(const std::vector<Operand>& ops,
                                        std::uint8_t load_op,
                                        std::uint8_t store_op,
                                        bool is_66) {
  if (ops.size() != 2) return;
  const auto l = [&]() -> std::uint8_t {
    for (const auto& op : ops) {
      if (op.kind == Operand::Kind::Register && is_ymm(op.reg.name)) return 1;
    }
    return 0;
  }();
  const std::uint8_t pp = is_66 ? 1 : 2;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register &&
      is_xmm_or_ymm(ops[0].reg.name) && is_xmm_or_ymm(ops[1].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[1].reg.name), false, needs_vex_ext(ops[0].reg.name), 1, 0, 0, l, pp);
    bytes.push_back(load_op);
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register &&
             is_xmm_or_ymm(ops[1].reg.name)) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[1].reg.name),
             ops[0].mem.index.has_value() && needs_vex_ext(ops[0].mem.index->name),
             ops[0].mem.base.has_value() && needs_vex_ext(ops[0].mem.base->name),
             1, 0, 0, l, pp);
    bytes.push_back(load_op);
    encode_modrm_mem(dst_num, ops[0].mem);
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory &&
             is_xmm_or_ymm(ops[0].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[0].reg.name),
             ops[1].mem.index.has_value() && needs_vex_ext(ops[1].mem.index->name),
             ops[1].mem.base.has_value() && needs_vex_ext(ops[1].mem.base->name),
             1, 0, 0, l, pp);
    bytes.push_back(store_op);
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_avx_mov_np(const std::vector<Operand>& ops,
                                           std::uint8_t load_op,
                                           std::uint8_t store_op,
                                           bool is_66) {
  if (ops.size() != 2) return;
  const auto l = [&]() -> std::uint8_t {
    for (const auto& op : ops) {
      if (op.kind == Operand::Kind::Register && is_ymm(op.reg.name)) return 1;
    }
    return 0;
  }();
  const std::uint8_t pp = is_66 ? 1 : 0;
  if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Register &&
      is_xmm_or_ymm(ops[0].reg.name) && is_xmm_or_ymm(ops[1].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[1].reg.name), false, needs_vex_ext(ops[0].reg.name), 1, 0, 0, l, pp);
    bytes.push_back(load_op);
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory && ops[1].kind == Operand::Kind::Register &&
             is_xmm_or_ymm(ops[1].reg.name)) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[1].reg.name),
             ops[0].mem.index.has_value() && needs_vex_ext(ops[0].mem.index->name),
             ops[0].mem.base.has_value() && needs_vex_ext(ops[0].mem.base->name),
             1, 0, 0, l, pp);
    bytes.push_back(load_op);
    encode_modrm_mem(dst_num, ops[0].mem);
  } else if (ops[0].kind == Operand::Kind::Register && ops[1].kind == Operand::Kind::Memory &&
             is_xmm_or_ymm(ops[0].reg.name)) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    emit_vex(needs_vex_ext(ops[0].reg.name),
             ops[1].mem.index.has_value() && needs_vex_ext(ops[1].mem.index->name),
             ops[1].mem.base.has_value() && needs_vex_ext(ops[1].mem.base->name),
             1, 0, 0, l, pp);
    bytes.push_back(store_op);
    encode_modrm_mem(src_num, ops[1].mem);
  }
}

void InstructionEncoder::encode_avx_3op(const std::vector<Operand>& ops,
                                        std::uint8_t opcode,
                                        bool has_66) {
  if (ops.size() != 3) return;
  const auto l = [&]() -> std::uint8_t {
    for (const auto& op : ops) {
      if (op.kind == Operand::Kind::Register && is_ymm(op.reg.name)) return 1;
    }
    return 0;
  }();
  const std::uint8_t pp = has_66 ? 1 : 0;
  if (ops[0].kind == Operand::Kind::Register &&
      ops[1].kind == Operand::Kind::Register &&
      ops[2].kind == Operand::Kind::Register) {
    const auto src_num = reg_num(ops[0].reg.name).value_or(0);
    const auto vvvv_num = reg_num(ops[1].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[2].reg.name).value_or(0);
    const auto vvvv_enc = static_cast<std::uint8_t>(vvvv_num | (needs_vex_ext(ops[1].reg.name) ? 8 : 0));
    emit_vex(needs_vex_ext(ops[2].reg.name), false, needs_vex_ext(ops[0].reg.name), 1, 0, vvvv_enc, l, pp);
    bytes.push_back(opcode);
    bytes.push_back(modrm(3, dst_num, src_num));
  } else if (ops[0].kind == Operand::Kind::Memory &&
             ops[1].kind == Operand::Kind::Register &&
             ops[2].kind == Operand::Kind::Register) {
    const auto vvvv_num = reg_num(ops[1].reg.name).value_or(0);
    const auto dst_num = reg_num(ops[2].reg.name).value_or(0);
    const auto vvvv_enc = static_cast<std::uint8_t>(vvvv_num | (needs_vex_ext(ops[1].reg.name) ? 8 : 0));
    emit_vex(needs_vex_ext(ops[2].reg.name),
             ops[0].mem.index.has_value() && needs_vex_ext(ops[0].mem.index->name),
             ops[0].mem.base.has_value() && needs_vex_ext(ops[0].mem.base->name),
             1, 0, vvvv_enc, l, pp);
    bytes.push_back(opcode);
    encode_modrm_mem(dst_num, ops[0].mem);
  }
}

void InstructionEncoder::emit_vex(bool r, bool x, bool b, std::uint8_t mm, std::uint8_t w,
                                  std::uint8_t vvvv, std::uint8_t l, std::uint8_t pp) {
  const std::uint8_t r_bit = r ? 0 : 1;
  const std::uint8_t x_bit = x ? 0 : 1;
  const std::uint8_t b_bit = b ? 0 : 1;
  const std::uint8_t vvvv_inv = static_cast<std::uint8_t>((~vvvv) & 0xF);
  if (mm == 1 && w == 0 && !x && !b) {
    bytes.push_back(0xC5);
    bytes.push_back(static_cast<std::uint8_t>((r_bit << 7) | (vvvv_inv << 3) | (l << 2) | pp));
  } else {
    bytes.push_back(0xC4);
    bytes.push_back(static_cast<std::uint8_t>((r_bit << 7) | (x_bit << 6) | (b_bit << 5) | mm));
    bytes.push_back(static_cast<std::uint8_t>((w << 7) | (vvvv_inv << 3) | (l << 2) | pp));
  }
}

}  // namespace c4c::backend::x86::assembler::encoder
