#include "x86_codegen.hpp"

#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace c4c::backend::x86 {

enum class AsmOperandKind : unsigned {
  GpReg,
  FpReg,
  Memory,
  Specific,
  Tied,
  Immediate,
  ConditionCode,
  X87St0,
  X87St1,
  QReg,
};

struct AsmOperand {
  AsmOperandKind kind = AsmOperandKind::GpReg;
  std::string reg;
  std::optional<std::string> name = std::nullopt;
  std::string mem_addr;
  std::optional<std::int64_t> imm_value = std::nullopt;
  std::optional<std::string> imm_symbol = std::nullopt;
  IrType operand_type = IrType::I64;
  std::string constraint;
  std::string seg_prefix;
};

namespace {

constexpr const char* kX86GpScratch[] = {"rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"};
constexpr const char* kX86XmmScratch[] = {"xmm0",  "xmm1",  "xmm2",  "xmm3",
                                          "xmm4",  "xmm5",  "xmm6",  "xmm7",
                                          "xmm8",  "xmm9",  "xmm10", "xmm11",
                                          "xmm12", "xmm13", "xmm14", "xmm15"};

struct ScratchState {
  std::size_t gp = 0;
  std::size_t xmm = 0;
};

ScratchState& scratch_state(const X86Codegen* codegen) {
  static std::unordered_map<const X86Codegen*, ScratchState> states;
  return states[codegen];
}

std::string reg_text(std::string_view reg) { return "%" + std::string(reg); }

std::string strip_constraint_prefix(std::string_view constraint) {
  std::size_t index = 0;
  while (index < constraint.size()) {
    const char ch = constraint[index];
    if (ch != '=' && ch != '+' && ch != '&' && ch != '%') {
      break;
    }
    ++index;
  }
  return std::string(constraint.substr(index));
}

std::optional<std::size_t> tied_operand_index(std::string_view constraint) {
  const std::string stripped = strip_constraint_prefix(constraint);
  if (stripped.empty()) {
    return std::nullopt;
  }
  for (const char ch : stripped) {
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return std::nullopt;
    }
  }
  return static_cast<std::size_t>(std::stoul(stripped));
}

std::optional<std::string> condition_code_suffix(std::string_view constraint) {
  const std::string stripped = strip_constraint_prefix(constraint);
  constexpr std::string_view prefix = "@cc";
  if (stripped.compare(0, prefix.size(), prefix.data(), prefix.size()) != 0) {
    return std::nullopt;
  }
  return stripped.substr(prefix.size());
}

std::string join_line(std::string_view mnemonic,
                      std::string_view lhs,
                      std::string_view rhs = std::string_view()) {
  if (rhs.empty()) {
    return std::string(mnemonic) + " " + std::string(lhs);
  }
  return std::string(mnemonic) + " " + std::string(lhs) + ", " + std::string(rhs);
}

void emit_instr_reg(X86CodegenState& state, std::string_view mnemonic, std::string_view reg) {
  state.emit(join_line(mnemonic, reg_text(reg)));
}

void emit_instr_reg_reg(X86CodegenState& state,
                        std::string_view mnemonic,
                        std::string_view lhs,
                        std::string_view rhs) {
  state.emit(join_line(mnemonic, reg_text(lhs), reg_text(rhs)));
}

void emit_text_with_reg(X86CodegenState& state,
                        std::string_view mnemonic,
                        std::string_view lhs,
                        std::string_view rhs) {
  state.emit(join_line(mnemonic, lhs, rhs));
}

std::string slot_operand(StackSlot slot) { return std::to_string(slot.raw) + "(%rbp)"; }

std::string memory_operand(std::string_view reg) { return "(" + reg_text(reg) + ")"; }

std::string src_reg_for_type(std::string_view reg, IrType ty) {
  switch (ty) {
    case IrType::I8: return reg_text(X86Codegen::reg_to_8l(std::string(reg)));
    case IrType::I16:
    case IrType::U16: return reg_text(X86Codegen::reg_to_16(std::string(reg)));
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return reg_text(X86Codegen::reg_to_32(std::string(reg)));
    default: return reg_text(reg);
  }
}

const char* fld_instr_for_type(IrType ty) {
  switch (ty) {
    case IrType::F32: return "flds";
    case IrType::F128: return "fldt";
    default: return "fldl";
  }
}

const char* fstp_instr_for_type(IrType ty) {
  switch (ty) {
    case IrType::F32: return "fstps";
    case IrType::F128: return "fstpt";
    default: return "fstpl";
  }
}

const char* xmm_move_instr_for_type(IrType ty) {
  switch (ty) {
    case IrType::F32: return "movss";
    case IrType::F64: return "movsd";
    default: return "movdqu";
  }
}

bool is_xmm_reg(std::string_view reg) {
  return reg.size() >= 3 && reg.substr(0, 3) == "xmm";
}

}  // namespace

AsmOperandKind X86Codegen::classify_constraint(const std::string& constraint) {
  const std::string c = strip_constraint_prefix(constraint);
  if (c.size() >= 2 && c.front() == '{' && c.back() == '}') {
    return AsmOperandKind::Specific;
  }
  if (condition_code_suffix(constraint).has_value()) {
    return AsmOperandKind::ConditionCode;
  }
  if (tied_operand_index(constraint).has_value()) {
    return AsmOperandKind::Tied;
  }
  if (c == "t") {
    return AsmOperandKind::X87St0;
  }
  if (c == "u") {
    return AsmOperandKind::X87St1;
  }

  bool has_gp = false;
  bool has_fp = false;
  bool has_mem = false;
  bool has_imm = false;
  bool has_qreg = false;
  bool has_specific = false;

  for (const char ch : c) {
    switch (ch) {
      case 'r':
      case 'q':
      case 'R':
      case 'l':
        has_gp = true;
        break;
      case 'Q':
        has_qreg = true;
        break;
      case 'g':
        has_gp = true;
        has_mem = true;
        has_imm = true;
        break;
      case 'x':
      case 'v':
      case 'Y':
        has_fp = true;
        break;
      case 'm':
      case 'o':
      case 'V':
      case 'p':
        has_mem = true;
        break;
      case 'i':
      case 'I':
      case 'n':
      case 'N':
      case 'e':
      case 'E':
      case 'K':
      case 'M':
      case 'G':
      case 'H':
      case 'J':
      case 'L':
      case 'O':
        has_imm = true;
        break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'S':
      case 'D':
      case 'A':
        has_specific = true;
        break;
      default:
        break;
    }
  }

  if (has_specific) {
    return AsmOperandKind::Specific;
  }
  if (has_gp) {
    return AsmOperandKind::GpReg;
  }
  if (has_qreg) {
    return AsmOperandKind::QReg;
  }
  if (has_fp) {
    return AsmOperandKind::FpReg;
  }
  if (has_mem) {
    return AsmOperandKind::Memory;
  }
  if (has_imm) {
    return AsmOperandKind::Immediate;
  }
  return AsmOperandKind::GpReg;
}

void X86Codegen::setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output) {
  (void)is_output;
  if (op.kind == AsmOperandKind::Memory) {
    if (const auto slot = this->state.get_slot(val.raw)) {
      if (this->state.is_alloca(val.raw) && !this->state.alloca_over_align(val.raw).has_value()) {
        op.mem_addr = slot_operand(*slot);
      } else {
        op.mem_addr.clear();
      }
    }
  }
  if (op.kind == AsmOperandKind::Immediate && val.immediate.has_value()) {
    op.imm_value = *val.immediate;
  }
}

bool X86Codegen::resolve_memory_operand(AsmOperand& op, const Operand& val, const std::vector<std::string>& excluded) {
  if (!op.mem_addr.empty()) {
    return false;
  }
  if (op.imm_symbol.has_value()) {
    op.mem_addr = *op.imm_symbol + "(%rip)";
    return false;
  }
  if (const auto slot = this->state.get_slot(val.raw)) {
    const std::string tmp_reg = this->assign_scratch_reg(AsmOperandKind::GpReg, excluded);
    if (this->state.is_alloca(val.raw)) {
      if (const auto align = this->state.alloca_over_align(val.raw)) {
        this->state.out.emit_instr_rbp_reg("    leaq", slot->raw, tmp_reg.c_str());
        this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(*align - 1),
                                           tmp_reg.c_str());
        this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(*align),
                                           tmp_reg.c_str());
      } else {
        this->state.out.emit_instr_rbp_reg("    leaq", slot->raw, tmp_reg.c_str());
      }
    } else {
      this->state.out.emit_instr_rbp_reg("    movq", slot->raw, tmp_reg.c_str());
    }
    op.mem_addr = memory_operand(tmp_reg);
    return true;
  }
  if (val.immediate.has_value()) {
    const std::string tmp_reg = this->assign_scratch_reg(AsmOperandKind::GpReg, excluded);
    this->state.out.emit_instr_imm_reg("    movabsq", *val.immediate, tmp_reg.c_str());
    op.mem_addr = memory_operand(tmp_reg);
    return true;
  }
  return false;
}

std::string X86Codegen::assign_scratch_reg(const AsmOperandKind& kind, const std::vector<std::string>& excluded) {
  auto is_excluded = [&excluded](std::string_view reg) {
    for (const auto& item : excluded) {
      if (item == reg) {
        return true;
      }
    }
    return false;
  };

  auto& state = scratch_state(this);
  if (kind == AsmOperandKind::FpReg) {
    while (state.xmm < std::size(kX86XmmScratch)) {
      const char* candidate = kX86XmmScratch[state.xmm++];
      if (!is_excluded(candidate)) {
        return candidate;
      }
    }
    for (const char* candidate : kX86XmmScratch) {
      if (!is_excluded(candidate)) {
        return candidate;
      }
    }
    return "xmm0";
  }
  if (kind == AsmOperandKind::QReg) {
    constexpr const char* kLegacyRegs[] = {"rax", "rbx", "rcx", "rdx"};
    for (const char* candidate : kLegacyRegs) {
      if (!is_excluded(candidate)) {
        return candidate;
      }
    }
    return "rax";
  }

  while (true) {
    const std::size_t index = state.gp++;
    std::string candidate;
    if (index < std::size(kX86GpScratch)) {
      candidate = kX86GpScratch[index];
    } else {
      const std::size_t extra = index - std::size(kX86GpScratch) + 12;
      if (extra > 15) {
        return "rcx";
      }
      candidate = "r" + std::to_string(extra);
    }
    if (!is_excluded(candidate)) {
      return candidate;
    }
  }
}

void X86Codegen::load_input_to_reg(const AsmOperand& op, const Operand& val, const std::string& constraint) {
  (void)constraint;
  const std::string& reg = op.reg;
  const IrType ty = op.operand_type;

  if (op.kind == AsmOperandKind::X87St0 || op.kind == AsmOperandKind::X87St1) {
    if (const auto slot = this->state.get_slot(val.raw)) {
      this->state.emit(join_line(fld_instr_for_type(ty), slot_operand(*slot)));
    } else if (val.immediate.has_value()) {
      this->state.emit("    subq $8, %rsp");
      this->state.out.emit_instr_imm_reg("    movabsq", *val.immediate, "rax");
      this->state.emit("    movq %rax, (%rsp)");
      this->state.emit(join_line(ty == IrType::F32 ? "flds" : "fldl", "(%rsp)"));
      this->state.emit("    addq $8, %rsp");
    }
    return;
  }

  if (is_xmm_reg(reg)) {
    if (const auto slot = this->state.get_slot(val.raw)) {
      this->state.emit(join_line(xmm_move_instr_for_type(ty), slot_operand(*slot), reg_text(reg)));
    } else {
      emit_instr_reg_reg(this->state, "    xorpd", reg, reg);
    }
    return;
  }

  if (val.immediate.has_value()) {
    const std::int64_t imm = *val.immediate;
    const std::string reg64 = X86Codegen::reg_to_64(reg);
    const std::string reg32 = X86Codegen::reg_to_32(reg64);
    const char* effective_reg = reg64 == reg ? reg.c_str() : reg64.c_str();
    if (imm == 0) {
      emit_instr_reg_reg(this->state, "    xorl", reg32, reg32);
    } else if (imm >= std::numeric_limits<std::int32_t>::min() &&
               imm <= std::numeric_limits<std::int32_t>::max()) {
      this->state.out.emit_instr_imm_reg("    movq", imm, effective_reg);
    } else if (imm >= 0 && imm <= std::numeric_limits<std::uint32_t>::max()) {
      this->state.out.emit_instr_imm_reg("    movl", imm, reg32.c_str());
    } else {
      this->state.out.emit_instr_imm_reg("    movabsq", imm, effective_reg);
    }
    return;
  }

  if (const auto slot = this->state.get_slot(val.raw)) {
    if (this->state.is_alloca(val.raw)) {
      const std::string reg64 = X86Codegen::reg_to_64(reg);
      const char* lea_reg = reg64 == reg ? reg.c_str() : reg64.c_str();
      if (const auto align = this->state.alloca_over_align(val.raw)) {
        this->state.out.emit_instr_rbp_reg("    leaq", slot->raw, lea_reg);
        this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(*align - 1),
                                           lea_reg);
        this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(*align),
                                           lea_reg);
      } else {
        this->state.out.emit_instr_rbp_reg("    leaq", slot->raw, lea_reg);
      }
      return;
    }
    const char* load_instr = this->mov_load_for_type(ty);
    const std::string dest_reg = (ty == IrType::U32 || ty == IrType::F32)
                                     ? reg_text(X86Codegen::reg_to_32(reg))
                                     : reg_text(reg);
    emit_text_with_reg(this->state, std::string("    ") + load_instr, slot_operand(*slot), dest_reg);
  }
}

void X86Codegen::preload_readwrite_output(const AsmOperand& op, const Value& ptr) {
  const IrType ty = op.operand_type;
  if (op.kind == AsmOperandKind::X87St0 || op.kind == AsmOperandKind::X87St1) {
    if (const auto slot = this->state.get_slot(ptr.raw)) {
      if (this->state.is_alloca(ptr.raw)) {
        this->state.emit(join_line(fld_instr_for_type(ty), slot_operand(*slot)));
      } else {
        emit_instr_reg(this->state, "    pushq", "rcx");
        this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rcx");
        this->state.emit(join_line(fld_instr_for_type(ty), "(%rcx)"));
        emit_instr_reg(this->state, "    popq", "rcx");
      }
    }
    return;
  }

  if (const auto slot = this->state.get_slot(ptr.raw)) {
    if (this->state.is_alloca(ptr.raw)) {
      if (is_xmm_reg(op.reg)) {
        this->state.emit(join_line(xmm_move_instr_for_type(ty), slot_operand(*slot),
                                   reg_text(op.reg)));
      } else {
        const char* load_instr = this->mov_load_for_type(ty);
        emit_text_with_reg(this->state, std::string("    ") + load_instr, slot_operand(*slot),
                           src_reg_for_type(op.reg, ty));
      }
      return;
    }

    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, op.reg.c_str());
    if (is_xmm_reg(op.reg)) {
      this->state.emit(join_line(xmm_move_instr_for_type(ty), memory_operand(op.reg),
                                 reg_text(op.reg)));
    } else {
      const char* load_instr = this->mov_load_for_type(ty);
      emit_text_with_reg(this->state, std::string("    ") + load_instr, memory_operand(op.reg),
                         src_reg_for_type(op.reg, ty));
    }
  }
}

std::string X86Codegen::substitute_template_line(const std::string& line,
                                                 const std::vector<AsmOperand>& operands,
                                                 const std::vector<std::size_t>& gcc_to_internal,
                                                 const std::vector<IrType>& operand_types,
                                                 const std::vector<std::pair<std::string, BlockId>>& goto_labels) {
  std::vector<std::string> op_regs;
  std::vector<std::optional<std::string>> op_names;
  std::vector<bool> op_is_memory;
  std::vector<std::string> op_mem_addrs;
  std::vector<std::optional<std::int64_t>> op_imm_values;
  std::vector<std::optional<std::string>> op_imm_symbols;
  op_regs.reserve(operands.size());
  op_names.reserve(operands.size());
  op_is_memory.reserve(operands.size());
  op_mem_addrs.reserve(operands.size());
  op_imm_values.reserve(operands.size());
  op_imm_symbols.reserve(operands.size());

  for (const auto& operand : operands) {
    op_regs.push_back(operand.reg);
    op_names.push_back(operand.name);
    op_is_memory.push_back(operand.kind == AsmOperandKind::Memory);
    op_mem_addrs.push_back(operand.seg_prefix.empty() ? operand.mem_addr
                                                      : operand.seg_prefix + operand.mem_addr);
    op_imm_values.push_back(operand.imm_value);
    op_imm_symbols.push_back(operand.imm_symbol);
  }

  std::vector<IrType> op_types(operands.size(), IrType::I64);
  for (std::size_t index = 0; index < operand_types.size() && index < op_types.size(); ++index) {
    op_types[index] = operand_types[index];
  }
  for (std::size_t index = 0; index < operands.size(); ++index) {
    if (operands[index].kind != AsmOperandKind::Tied) {
      continue;
    }
    if (const auto tied_to = tied_operand_index(operands[index].constraint)) {
      if (*tied_to < op_types.size()) {
        op_types[index] = op_types[*tied_to];
      }
    }
  }

  return X86Codegen::substitute_x86_asm_operands(line, op_regs, op_names, op_is_memory,
                                                 op_mem_addrs, op_types, gcc_to_internal,
                                                 goto_labels, op_imm_values, op_imm_symbols);
}

void X86Codegen::store_output_from_reg(const AsmOperand& op,
                                       const Value& ptr,
                                       const std::string& constraint,
                                       const std::vector<const char*>& all_output_regs) {
  (void)constraint;
  (void)all_output_regs;
  if (op.kind == AsmOperandKind::Memory) {
    return;
  }
  if (const auto slot = this->state.get_slot(ptr.raw)) {
    if (op.kind == AsmOperandKind::X87St0 || op.kind == AsmOperandKind::X87St1) {
      if (this->state.is_alloca(ptr.raw)) {
        this->state.emit(join_line(fstp_instr_for_type(op.operand_type), slot_operand(*slot)));
      } else {
        emit_instr_reg(this->state, "    pushq", "rcx");
        this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rcx");
        this->state.emit(join_line(fstp_instr_for_type(op.operand_type), "(%rcx)"));
        emit_instr_reg(this->state, "    popq", "rcx");
      }
      return;
    }

    if (op.kind == AsmOperandKind::ConditionCode) {
      const std::string cond = condition_code_suffix(op.constraint).value_or("e");
      const std::string reg8 = X86Codegen::reg_to_8l(op.reg);
      this->state.emit("    set" + std::string(X86Codegen::gcc_cc_to_x86(cond)) + " " +
                       reg_text(reg8));
      emit_instr_reg_reg(this->state, "    movzbl", reg8, X86Codegen::reg_to_32(op.reg));
      const char* store_instr = this->mov_store_for_type(op.operand_type);
      if (this->state.is_alloca(ptr.raw)) {
        emit_text_with_reg(this->state, std::string("    ") + store_instr,
                           src_reg_for_type(op.reg, op.operand_type), slot_operand(*slot));
      } else {
        const char* scratch = op.reg == "rcx" ? "rdx" : "rcx";
        emit_instr_reg(this->state, "    pushq", scratch);
        this->state.out.emit_instr_rbp_reg("    movq", slot->raw, scratch);
        emit_text_with_reg(this->state, std::string("    ") + store_instr,
                           src_reg_for_type(op.reg, op.operand_type), memory_operand(scratch));
        emit_instr_reg(this->state, "    popq", scratch);
      }
      return;
    }

    if (is_xmm_reg(op.reg)) {
      const char* store_instr = xmm_move_instr_for_type(op.operand_type);
      if (this->state.is_alloca(ptr.raw)) {
        this->state.emit(join_line(store_instr, reg_text(op.reg), slot_operand(*slot)));
      } else {
        emit_instr_reg(this->state, "    pushq", "rcx");
        this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rcx");
        this->state.emit(join_line(store_instr, reg_text(op.reg), "(%rcx)"));
        emit_instr_reg(this->state, "    popq", "rcx");
      }
      return;
    }

    const char* store_instr = this->mov_store_for_type(op.operand_type);
    if (this->state.is_alloca(ptr.raw)) {
      emit_text_with_reg(this->state, std::string("    ") + store_instr,
                         src_reg_for_type(op.reg, op.operand_type), slot_operand(*slot));
    } else {
      const char* scratch = op.reg == "rcx" ? "rdx" : "rcx";
      emit_instr_reg(this->state, "    pushq", scratch);
      this->state.out.emit_instr_rbp_reg("    movq", slot->raw, scratch);
      emit_text_with_reg(this->state, std::string("    ") + store_instr,
                         src_reg_for_type(op.reg, op.operand_type), memory_operand(scratch));
      emit_instr_reg(this->state, "    popq", scratch);
    }
  }
}

void X86Codegen::reset_scratch_state() { scratch_state(this) = ScratchState{}; }

}  // namespace c4c::backend::x86
