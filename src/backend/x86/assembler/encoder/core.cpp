#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Core encoding helpers for x86-64 instruction emission.

namespace c4c::backend::x86::assembler::encoder {

std::uint8_t InstructionEncoder::rex(bool w, bool r, bool x, bool b) const {
  std::uint8_t rex = 0x40u;
  if (w) rex |= 0x08;
  if (r) rex |= 0x04;
  if (x) rex |= 0x02;
  if (b) rex |= 0x01;
  return rex;
}

std::uint8_t InstructionEncoder::modrm(std::uint8_t mod_, std::uint8_t reg, std::uint8_t rm) const {
  return static_cast<std::uint8_t>((mod_ << 6) | ((reg & 7) << 3) | (rm & 7));
}

std::uint8_t InstructionEncoder::sib(std::uint8_t scale, std::uint8_t index, std::uint8_t base) const {
  std::uint8_t scale_bits = 0;
  switch (scale) {
    case 1: scale_bits = 0; break;
    case 2: scale_bits = 1; break;
    case 4: scale_bits = 2; break;
    case 8: scale_bits = 3; break;
    default: scale_bits = 0; break;
  }
  return static_cast<std::uint8_t>((scale_bits << 6) | ((index & 7) << 3) | (base & 7));
}

void InstructionEncoder::emit_rex_rr(std::uint8_t size, const std::string& reg, const std::string& rm) {
  const bool w = size == 8;
  const bool r = needs_rex_ext(reg);
  const bool b = needs_rex_ext(rm);
  const bool need_rex = w || r || b || is_rex_required_8bit(reg) || is_rex_required_8bit(rm);
  if (need_rex) {
    bytes.push_back(rex(w, r, false, b));
  }
}

void InstructionEncoder::emit_segment_prefix(const MemoryOperand& mem) {
  if (mem.segment.has_value()) {
    if (*mem.segment == "fs") {
      bytes.push_back(0x64);
    } else if (*mem.segment == "gs") {
      bytes.push_back(0x65);
    }
  }
}

void InstructionEncoder::emit_rex_rm(std::uint8_t size, const std::string& reg, const MemoryOperand& mem) {
  const bool w = size == 8;
  const bool r = needs_rex_ext(reg);
  const bool b = mem.base.has_value() && needs_rex_ext(mem.base->name);
  const bool x = mem.index.has_value() && needs_rex_ext(mem.index->name);
  const bool need_rex = w || r || b || x || is_rex_required_8bit(reg);
  if (need_rex) {
    bytes.push_back(rex(w, r, x, b));
  }
}

void InstructionEncoder::emit_rex_unary(std::uint8_t size, const std::string& rm) {
  const bool w = size == 8;
  const bool b = needs_rex_ext(rm);
  const bool need_rex = w || b || is_rex_required_8bit(rm);
  if (need_rex) {
    bytes.push_back(rex(w, false, false, b));
  }
}

void InstructionEncoder::add_relocation(const std::string& symbol, std::uint32_t reloc_type, std::int64_t addend) {
  const auto base = symbol.rfind("@PLT") == symbol.size() - 4 ? symbol.substr(0, symbol.size() - 4) : symbol;
  const auto rtype = (base != symbol && reloc_type == 2) ? 4u : reloc_type;
  relocations.push_back(Relocation{
      .offset = offset + static_cast<std::uint64_t>(bytes.size()),
      .symbol = base,
      .reloc_type = rtype,
      .addend = addend,
  });
}

void InstructionEncoder::encode_modrm_mem(std::uint8_t reg_field, const MemoryOperand& mem) {
  const auto base = mem.base;
  const auto index = mem.index;
  if (base.has_value() && base->name == "rip") {
    bytes.push_back(modrm(0, reg_field, 5));
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    return;
  }

  const std::string base_reg = base.has_value() ? base->name : std::string();
  const std::uint8_t base_num = base.has_value() ? reg_num(base_reg).value_or(0) : 5;
  const bool need_sib = index.has_value() || ((base_num & 7) == 4) || !base.has_value();
  const bool has_symbol = mem.displacement.kind == Displacement::Kind::Symbol ||
                          mem.displacement.kind == Displacement::Kind::SymbolAddend ||
                          mem.displacement.kind == Displacement::Kind::SymbolMod ||
                          mem.displacement.kind == Displacement::Kind::SymbolPlusOffset;

  std::int64_t disp_val = 0;
  if (mem.displacement.kind == Displacement::Kind::Integer) {
    disp_val = mem.displacement.integer;
  }

  const std::uint8_t mod_bits = has_symbol ? 2 :
      (disp_val == 0 && (base_num & 7) != 5 ? 0 : ((disp_val >= -128 && disp_val <= 127) ? 1 : 2));
  const std::size_t disp_size = has_symbol ? 4 : (mod_bits == 0 ? 0 : (mod_bits == 1 ? 1 : 4));

  if (need_sib) {
    const std::uint8_t idx_num = index.has_value() ? reg_num(index->name).value_or(4) : 4;
    const std::uint8_t scale = mem.scale.value_or(1);
    if (!base.has_value()) {
      bytes.push_back(modrm(0, reg_field, 4));
      bytes.push_back(sib(scale, idx_num, 5));
      bytes.insert(bytes.end(), 4, 0);
      return;
    }
    bytes.push_back(modrm(mod_bits, reg_field, 4));
    bytes.push_back(sib(scale, idx_num, base_num));
    if (disp_size == 1) bytes.push_back(static_cast<std::uint8_t>(disp_val));
    else if (disp_size == 4) {
      const auto b = static_cast<std::int32_t>(disp_val);
      const auto* p = reinterpret_cast<const std::uint8_t*>(&b);
      bytes.insert(bytes.end(), p, p + 4);
    }
    return;
  }

  bytes.push_back(modrm(mod_bits, reg_field, base_num));
  if (disp_size == 1) bytes.push_back(static_cast<std::uint8_t>(disp_val));
  else if (disp_size == 4) {
    const auto b = static_cast<std::int32_t>(disp_val);
    const auto* p = reinterpret_cast<const std::uint8_t*>(&b);
    bytes.insert(bytes.end(), p, p + 4);
  }
}

void InstructionEncoder::adjust_rip_reloc_addend(std::size_t reloc_count_before, std::int64_t trailing_bytes) {
  if (relocations.size() > reloc_count_before) {
    auto& reloc = relocations[reloc_count_before];
    reloc.addend -= trailing_bytes;
  }
}

void InstructionEncoder::encode(const Instruction& instr) {
  const auto start_len = bytes.size();
  if (instr.prefix.has_value()) {
    if (*instr.prefix == "lock") bytes.push_back(0xF0);
    else if (*instr.prefix == "rep" || *instr.prefix == "repz" || *instr.prefix == "repe") bytes.push_back(0xF3);
    else if (*instr.prefix == "repnz" || *instr.prefix == "repne") bytes.push_back(0xF2);
    else if (*instr.prefix == "notrack") bytes.push_back(0x3E);
  }
  encode_mnemonic(instr);
  offset += static_cast<std::uint64_t>(bytes.size() - start_len);
}

void InstructionEncoder::encode_mnemonic(const Instruction& instr) {
  const auto mnemonic = infer_suffix(instr.mnemonic, instr.operands);
  const auto& ops = instr.operands;
  if (mnemonic == "movq") {
    encode_mov(ops, 8);
  } else if (mnemonic == "movl") {
    encode_mov(ops, 4);
  } else if (mnemonic == "movw") {
    encode_mov(ops, 2);
  } else if (mnemonic == "movb") {
    encode_mov(ops, 1);
  } else if (mnemonic == "movabsq") {
    encode_movabs(ops);
  } else if (mnemonic == "outb" || mnemonic == "outw" || mnemonic == "outl") {
    encode_out(ops, mnemonic);
  } else if (mnemonic == "inb" || mnemonic == "inw" || mnemonic == "inl") {
    encode_in(ops, mnemonic);
  } else if (mnemonic == "clflush") {
    encode_clflush(ops);
  } else if (mnemonic == "verw") {
    encode_verw(ops);
  } else if (mnemonic == "movcr" || mnemonic == "movcrq") {
    encode_mov_cr(ops);
  } else if (mnemonic == "movdr" || mnemonic == "movdrq") {
    encode_mov_dr(ops);
  } else if (mnemonic == "btq" || mnemonic == "btl" || mnemonic == "btw" || mnemonic == "btb") {
    encode_bt(ops, mnemonic);
  } else if (mnemonic == "fldt" || mnemonic == "fldl" || mnemonic == "flds") {
    encode_x87_mem(ops, std::vector<std::uint8_t>{0xD9}, 0);
  } else {
    // The remaining instruction families are translated in sibling files and
    // can be wired into this dispatch as their coverage grows.
  }
}

}  // namespace c4c::backend::x86::assembler::encoder
