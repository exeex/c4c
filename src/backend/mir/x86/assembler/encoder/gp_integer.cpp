#include <cstdint>
#include <string>
#include <vector>

// General-purpose integer instruction encoders.

namespace c4c::backend::x86::assembler::encoder {

void InstructionEncoder::encode_mov(const std::vector<Operand>& ops, std::uint8_t size) {
  if (ops.size() != 2) return;
  const auto& src = ops[0];
  const auto& dst = ops[1];
  if (src.kind == Operand::Kind::Immediate && dst.kind == Operand::Kind::Register) {
    encode_mov_imm_reg(src.imm, dst.reg, size);
  } else if (src.kind == Operand::Kind::Register && dst.kind == Operand::Kind::Register) {
    encode_mov_rr(src.reg, dst.reg, size);
  } else if (src.kind == Operand::Kind::Memory && dst.kind == Operand::Kind::Register) {
    encode_mov_mem_reg(src.mem, dst.reg, size);
  } else if (src.kind == Operand::Kind::Register && dst.kind == Operand::Kind::Memory) {
    encode_mov_reg_mem(src.reg, dst.mem, size);
  } else if (src.kind == Operand::Kind::Immediate && dst.kind == Operand::Kind::Memory) {
    encode_mov_imm_mem(src.imm, dst.mem, size);
  } else if (src.kind == Operand::Kind::Label && dst.kind == Operand::Kind::Register) {
    MemoryOperand mem;
    mem.displacement.kind = Displacement::Kind::Symbol;
    mem.displacement.a = src.label;
    encode_mov_mem_reg(mem, dst.reg, size);
  } else if (src.kind == Operand::Kind::Register && dst.kind == Operand::Kind::Label) {
    MemoryOperand mem;
    mem.displacement.kind = Displacement::Kind::Symbol;
    mem.displacement.a = dst.label;
    encode_mov_reg_mem(src.reg, mem, size);
  }
}

void InstructionEncoder::encode_mov_imm_reg(const ImmediateValue& imm, const Register& dst, std::uint8_t size) {
  const auto dst_num = reg_num(dst.name).value_or(0);
  if (imm.kind == ImmediateValue::Kind::Integer) {
    const auto val = imm.integer;
    if (size == 8) {
      if (val >= INT32_MIN && val <= INT32_MAX) {
        emit_rex_unary(8, dst.name);
        bytes.push_back(0xC7);
        bytes.push_back(modrm(3, 0, dst_num));
        const auto v = static_cast<std::int32_t>(val);
        const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
        bytes.insert(bytes.end(), p, p + 4);
      } else {
        bytes.push_back(rex(true, false, false, needs_rex_ext(dst.name)));
        bytes.push_back(static_cast<std::uint8_t>(0xB8 + (dst_num & 7)));
        const auto* p = reinterpret_cast<const std::uint8_t*>(&val);
        bytes.insert(bytes.end(), p, p + 8);
      }
    } else if (size == 4) {
      if (needs_rex_ext(dst.name)) bytes.push_back(rex(false, false, false, true));
      bytes.push_back(0xC7);
      bytes.push_back(modrm(3, 0, dst_num));
      const auto v = static_cast<std::int32_t>(val);
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes.insert(bytes.end(), p, p + 4);
    } else if (size == 2) {
      bytes.push_back(0x66);
      if (needs_rex_ext(dst.name)) bytes.push_back(rex(false, false, false, true));
      bytes.push_back(0xC7);
      bytes.push_back(modrm(3, 0, dst_num));
      const auto v = static_cast<std::int16_t>(val);
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes.insert(bytes.end(), p, p + 2);
    } else {
      if (needs_rex_ext(dst.name) || is_rex_required_8bit(dst.name)) {
        bytes.push_back(rex(false, false, false, needs_rex_ext(dst.name)));
      }
      bytes.push_back(0xC6);
      bytes.push_back(modrm(3, 0, dst_num));
      bytes.push_back(static_cast<std::uint8_t>(val));
    }
  } else if (imm.kind == ImmediateValue::Kind::Symbol || imm.kind == ImmediateValue::Kind::SymbolPlusOffset) {
    const auto addend = imm.kind == ImmediateValue::Kind::SymbolPlusOffset ? 0 : 0;
    if (size == 8) {
      emit_rex_unary(8, dst.name);
      bytes.push_back(0xC7);
      bytes.push_back(modrm(3, 0, dst_num));
      add_relocation(imm.a, 11, addend);
      bytes.insert(bytes.end(), 4, 0);
    } else {
      emit_rex_unary(size, dst.name);
      bytes.push_back(0xC7);
      bytes.push_back(modrm(3, 0, dst_num));
      add_relocation(imm.a, 10, addend);
      bytes.insert(bytes.end(), 4, 0);
    }
  }
}

void InstructionEncoder::encode_mov_rr(const Register& src, const Register& dst, std::uint8_t size) {
  const auto src_num = reg_num(src.name).value_or(0);
  const auto dst_num = reg_num(dst.name).value_or(0);
  if (size == 2) bytes.push_back(0x66);
  emit_rex_rr(size, src.name, dst.name);
  bytes.push_back(size == 1 ? 0x88 : 0x89);
  bytes.push_back(modrm(3, src_num, dst_num));
}

void InstructionEncoder::encode_mov_mem_reg(const MemoryOperand& mem, const Register& dst, std::uint8_t size) {
  const auto dst_num = reg_num(dst.name).value_or(0);
  emit_segment_prefix(mem);
  if (size == 2) bytes.push_back(0x66);
  emit_rex_rm(size, dst.name, mem);
  bytes.push_back(size == 1 ? 0x8A : 0x8B);
  encode_modrm_mem(dst_num, mem);
}

void InstructionEncoder::encode_mov_reg_mem(const Register& src, const MemoryOperand& mem, std::uint8_t size) {
  const auto src_num = reg_num(src.name).value_or(0);
  emit_segment_prefix(mem);
  if (size == 2) bytes.push_back(0x66);
  emit_rex_rm(size, src.name, mem);
  bytes.push_back(size == 1 ? 0x88 : 0x89);
  encode_modrm_mem(src_num, mem);
}

void InstructionEncoder::encode_mov_imm_mem(const ImmediateValue& imm, const MemoryOperand& mem, std::uint8_t size) {
  emit_segment_prefix(mem);
  if (size == 2) bytes.push_back(0x66);
  emit_rex_rm(size == 8 ? 8 : size, "", mem);
  bytes.push_back(size == 1 ? 0xC6 : 0xC7);
  const auto reloc_count = relocations.size();
  encode_modrm_mem(0, mem);
  if (imm.kind == ImmediateValue::Kind::Integer) {
    if (size == 1) {
      bytes.push_back(static_cast<std::uint8_t>(imm.integer));
    } else if (size == 2) {
      const auto v = static_cast<std::int16_t>(imm.integer);
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes.insert(bytes.end(), p, p + 2);
    } else {
      const auto v = static_cast<std::int32_t>(imm.integer);
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes.insert(bytes.end(), p, p + 4);
    }
  }
  adjust_rip_reloc_addend(reloc_count, size == 1 ? 1 : (size == 2 ? 2 : 4));
}

void InstructionEncoder::encode_movabs(const std::vector<Operand>& ops) {
  if (ops.size() != 2) return;
  if (ops[0].kind == Operand::Kind::Immediate && ops[1].kind == Operand::Kind::Register) {
    const auto dst_num = reg_num(ops[1].reg.name).value_or(0);
    bytes.push_back(rex(true, false, false, needs_rex_ext(ops[1].reg.name)));
    bytes.push_back(static_cast<std::uint8_t>(0xB8 + (dst_num & 7)));
    if (ops[0].imm.kind == ImmediateValue::Kind::Integer) {
      const auto* p = reinterpret_cast<const std::uint8_t*>(&ops[0].imm.integer);
      bytes.insert(bytes.end(), p, p + 8);
    } else {
      add_relocation(ops[0].imm.a, 1, 0);
      bytes.insert(bytes.end(), 8, 0);
    }
  }
}

void InstructionEncoder::encode_movsx(const std::vector<Operand>& ops, std::uint8_t, std::uint8_t) {
  (void)ops;
}

void InstructionEncoder::encode_movzx(const std::vector<Operand>& ops, std::uint8_t, std::uint8_t) {
  (void)ops;
}

}  // namespace c4c::backend::x86::assembler::encoder
